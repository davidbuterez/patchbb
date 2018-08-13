#ifndef SOURCEINFO_H
#define SOURCEINFO_H

#include <iostream>

class SourceInfo {
  // friend class SourceInfoCompare;
  public:
    SourceInfo(std::string filename, unsigned lineNo);
    std::string getFilename() const;
    unsigned getLineNo() const;
    // bool operator <(const SourceInfo&) const;
    friend std::ostream& operator <<(std::ostream&, const SourceInfo&);
  private:
    std::string filename;
    int lineNo;
};

#endif /* SOURCEINFO_H */
