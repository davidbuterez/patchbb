#!/usr/bin/env python3
import argparse
import subprocess
import sys
import os, os.path
import getpass
import shlex
import json
import glob
import pygit2
import datetime

# Returns a list of all bitcode files that do not have a function named 'main' (important in order to avoid conflicts)
def find_all_no_mains(name):
  files_with_no_main = []

  for root, dirs, files in os.walk(os.getcwd()):
    for f in files:
      fullpath = os.path.join(root, f)
      if os.path.splitext(fullpath)[1] == '.c':
        proc = subprocess.Popen(['universalctags', '-x', '--c-kinds=fp', '--fields=+ne', '--output-format=json', fullpath], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = proc.communicate()

        if err:
          exit(1)

        output = out.decode('utf-8').strip().split('\n')
        if output:
          has_main = False
          for fn in output:
            if fn:
              jsn = json.loads(fn)
              if jsn['name'] == 'main':
                has_main = True
          if not has_main:
            files_with_no_main.append(extract_path_of_interest(fullpath, name)[:-1] + 'o.bc')

  return list(filter(is_lib, files_with_no_main))

# Run extract-bc on all object files
def extract_bc():
  for root, dirs, files in os.walk(os.getcwd()):
    for f in files:
      fullpath = os.path.join(root, f)
      if os.path.splitext(fullpath)[1] == '.o':
        subprocess.call(['extract-bc', fullpath])

# Handles the cloning of a repo
def clone_repo(repo_url, name):
  repo = None

  try:
    repo = pygit2.clone_repository(repo_url, os.getcwd() + "/" + name)
  except pygit2.GitError:
    username = input('Enter git username: ')
    password = getpass.getpass('Enter git password: ')
    cred = pygit2.UserPass(username, password)
    try:
      repo = pygit2.clone_repository(repo_url,  os.getcwd() + "/" + name, callbacks=pygit2.RemoteCallbacks(credentials=cred))
    except ValueError:
      print("Invalid URL!")
      sys.exit(1)
  except Exception as e:
    print(e)
    sys.exit(1)

  return repo

# Do not need full path, return just the meaningful part
def extract_path_of_interest(fullpath, repo_name, source=False):
  idx = fullpath.find(repo_name)
  interest = fullpath[idx + len(repo_name) + 1:]
  if source and interest.endswith('o.bc'):
    return interest[:-4] + 'c'
  else:
    return interest

# Return true if it's a library function
def is_lib(fullpath):
  return fullpath.startswith('lib') or fullpath.startswith('gl')

def main(main_args):
  # Initialize argparse
  parser = argparse.ArgumentParser(description='TODO')

  parser.add_argument('gitrepo', help='git repo url')
  parser.add_argument('passpath')
  parser.add_argument('--name')
  parser.add_argument('--llvm-link-path', dest='link')
  parser.add_argument('--opt-path', dest='opt')
  parser.add_argument('--patch-history', dest='hist')

  # Dictionary of arguments
  args_orig = parser.parse_args(main_args)
  args = vars(args_orig)

  # Uncomment if repository is already cloned
  # repo = None
  # discover_path = pygit2.discover_repository(os.getcwd())
  # repo = pygit2.Repository(discover_path)

  repo = clone_repo(args['gitrepo'], args['name'])

  # Load JSON data from diffanalyze
  with open(args['hist']) as f:
    history = json.load(f)

  commits_reverse = reversed(list(history.keys()))
  commits_seen = 0

  # Enclosing directory
  enclosing = os.getcwd()

  for commit in commits_reverse:
    print('Looking at commit: %s' % (commit,))
    # Repo to desired revision
    os.chdir(args['name'])
    subprocess.call(['git', 'checkout', commit], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    # Check that we can run the script
    if not os.path.isfile('configure') and not os.path.isfile('bootstrap'):
      print('Skipping commit %s as it doesn\'t have build files - probably too old' % (commit,))
      os.chdir(enclosing)
      continue

    os.chdir(enclosing)
    updates = history[commit]

    # Build
    try:
      subprocess.check_call(['./build.sh', args['name']])
    except subprocess.CalledProcessError:
      print('There was an error building. Skipping commit...')
      continue

    # Run extract-bc and find all files without a 'main' function
    os.chdir(args['name'])
    extract_bc()
    no_mains = find_all_no_mains(args['name'])
    input_files = []

    # Compute list of bitcode files to give to llvm-link
    for root, dirs, files in os.walk(os.getcwd()):
      for f in files:
        fullpath = os.path.join(root, f)
        path = extract_path_of_interest(fullpath, args['name'])

        if os.path.splitext(fullpath)[1] == '.bc' and path in no_mains:
          input_files.append(path)

    # Call llvm-link and opt for each version
    for filename, updated_lines in updates.items():
      cmds = []
      
      if args['link']:
        cmds.append(args['link'])
      else:
        cmds.append('llvm-link')

      filename_bc = filename[:-1] + 'o.bc'
      if filename_bc not in no_mains:
        cmds.append(filename_bc)
      cmds.extend(input_files)
      cmds.append('-o')
      cmds.append('LINKED.bc')

      subprocess.call(cmds)

      upd, item = {}, {}
      item[filename] = updated_lines
      upd[commit] = item

      opt_cmds = []
      if args['opt']:
        opt_cmds.append(args['opt'])
      else:
        opt_cmds.append('opt')

      opt_cmds.append('-load')
      opt_cmds.append(args['passpath'])
      opt_cmds.append('-countBB')
      opt_cmds.append('-repo-name')
      opt_cmds.append(args['name'])
      opt_cmds.append('-repo-patches')
      opt_cmds.append(json.dumps(upd))
      opt_cmds.append('LINKED.bc')

      subprocess.call(opt_cmds)

    commits_seen += 1
    os.chdir(enclosing)
    print('Seen commit: %s' % (commit,))


if __name__ == '__main__':
  main(sys.argv[1:])