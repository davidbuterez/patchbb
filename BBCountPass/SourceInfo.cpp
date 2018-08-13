#include "SourceInfo.hpp"

SourceInfo::SourceInfo(std::string filename, unsigned lineNo) : filename(filename), lineNo(lineNo) {}

std::string SourceInfo::getFilename() const {
  return filename;
}

unsigned SourceInfo::getLineNo() const {
  return lineNo;
}


// bool SourceInfo::operator <(const SourceInfo& rhs) const {
//   int comp = filename.compare(rhs.filename);
//   return comp < 0 ? true : line_no < rhs.line_no; 
// }

std::ostream& operator <<(std::ostream &strm, const SourceInfo &srcInfo) {
  return strm << srcInfo.filename << ":" << srcInfo.lineNo;
}  
