#include <CCSS.h>
#include <CXML.h>
#include <CXMLParser.h>
#include <CFile.h>
#include <CStrUtil.h>
#include <CStrParse.h>
#include <cassert>

CCSS::
CCSS()
{
}

bool
CCSS::
processFile(const std::string &filename)
{
  if (! CFile::exists(filename) || ! CFile::isRegular(filename)) {
    if (isDebug())
      std::cerr << "Invalid file '" << filename << "'" << std::endl;
    return false;
  }

  CFile file(filename);

  std::string str;

  std::string line;

  while (file.readLine(line)) {
    line = CStrUtil::stripSpaces(line);

    if (line.empty())
      continue;

    if (! str.empty())
      str += "\n";

    str += line;
  }

  CXML xml;

  CXMLParser parser(xml);

  std::string str1 = parser.replaceNamedChars(str);

  processLine(str1);

  return true;
}

bool
CCSS::
processLine(const std::string &line)
{
  return parse(line);
}

void
CCSS::
getSelectors(std::vector<Selector> &selectors) const
{
  for (const auto &d : styleData_) {
    const Selector &selector = d.first;

    selectors.push_back(selector);
  }
}

bool
CCSS::
parse(const std::string &str)
{
  struct Id {
    std::string         id;
    SelectName::SubType subType { SelectName::SubType::NONE };
    std::string         subId;
  };

  typedef std::vector<Id>     IdList;
  typedef std::vector<IdList> IdListList;

  CStrParse parse(str);

  while (! parse.eof()) {
    parse.skipSpace();

    if (parse.isString("/*")) {
      skipComment(parse);

      parse.skipSpace();
    }

    // get ids
    IdListList idListList;

    while (! parse.eof()) {
      // read comma separated list of space separated ids
      while (! parse.eof()) {
        IdList idList;

        while (! parse.eof()) {
          Id id;

          if (! readId(parse, id.id)) {
            if (isDebug())
              std::cerr << "Empty id : '" << parse.stateStr() << "'" << std::endl;
            return false;
          }

          if      (parse.isChar('>')) {
            parse.skipChar();

            parse.skipSpace();

            if (! readId(parse, id.subId)) {
              if (isDebug())
                std::cerr << "Empty id : '" << parse.stateStr() << "'" << std::endl;
              return false;
            }

            id.subType = SelectName::SubType::CHILD;
          }
          else if (parse.isChar('+')) {
            parse.skipChar();

            parse.skipSpace();

            if (! readId(parse, id.subId)) {
              if (isDebug())
                std::cerr << "Empty id : '" << parse.stateStr() << "'" << std::endl;
              return false;
            }

            id.subType = SelectName::SubType::SIBLING;
          }
          else if (parse.isChar('~')) {
            parse.skipChar();

            parse.skipSpace();

            if (! readId(parse, id.subId)) {
              if (isDebug())
                std::cerr << "Empty id : '" << parse.stateStr() << "'" << std::endl;
              return false;
            }

            id.subType = SelectName::SubType::PRECEDER;
          }

          idList.push_back(id);

          if (parse.isChar(',') || parse.isChar('{'))
            break;
        }

        if (! idList.empty())
          idListList.push_back(idList);

        if (! parse.isChar(','))
          break;

        parse.skipChar();

        parse.skipSpace();
      }

      if (parse.isChar('{'))
        break;
    }

    //---

    if (! parse.isChar('{')) {
      if (isDebug())
        std::cerr << "Missing '{' for rule" << std::endl;
      return false;
    }

    std::string str1;

    // still parse text with missing end brace, just exit loop
    bool rc = readBracedString(parse, str1);

    StyleData styleData;

    if (! parseAttr(str1, styleData))
      return false;

    if (! rc)
      break;

    //---

    for (const auto &idList : idListList) {
      for (const auto &id : idList) {
        Selector selector;

        addSelector(selector, id.id, id.subType, id.subId);

        StyleData &styleData1 = getStyleData(selector);

        for (const auto &opt : styleData.getOptions()) {
          styleData1.addOption(opt);
        }
      }
    }
  }

  return true;
}

void
CCSS::
addSelector(Selector &selector, const std::string &id,
            SelectName::SubType subType, const std::string &subId)
{
  assert(! id.empty());

  std::string name = id;

  SelectName::Type type = SelectName::Type::TYPE;

  std::string className;

  // # <id>
  if      (name[0] == '#') {
    type = SelectName::Type::ID;
    name = name.substr(1);
  }
  // <name> [. <class> ]
  else {
    auto p = name.find('.');

    if (p != std::string::npos) {
      type      = SelectName::Type::CLASS;
      className = name.substr(p + 1);
      name      = name.substr(0, p);
    }
  }

  //---

  // <name> [ <expr> ]
  std::string expr;

  auto p = name.find('[');

  if (p != std::string::npos) {
    expr = name.substr(p + 1);
    name = name.substr(0, p);

    auto p1 = expr.find(']');

    if (p1 != std::string::npos)
      expr = expr.substr(0, p1);
  }

  //---

  // <name>:<fn>
  std::string fn;

  p = name.find(':');

  if (p != std::string::npos) {
    fn  = name.substr(p + 1);
    name = name.substr(0, p);
  }

  //---

  selector.addName(type, name, className, subType, subId);

  if (expr != "")
    selector.addExpr(expr);

  if (fn != "")
    selector.addFunction(fn);
}

bool
CCSS::
parseAttr(const std::string &str, StyleData &styleData)
{
  CStrParse parse(str);

  while (! parse.eof()) {
    std::string name;

    while (! parse.eof() && ! parse.isSpace() && ! parse.isChar(':')) {
      char c;

      parse.readChar(&c);

      name += c;
    }

    parse.skipSpace();

    std::string value;

    if (parse.isChar(':')) {
      parse.skipChar();

      parse.skipSpace();

      while (! parse.eof() && ! parse.isChar(';')) {
        char c;

        parse.readChar(&c);

        value += c;
      }

      if (parse.isChar(';')) {
        parse.skipChar();

        parse.skipSpace();
      }
    }

    if (name.empty()) {
      if (isDebug())
        std::cerr << "Empty name : '" << parse.stateStr() << "'" << std::endl;
      return false;
    }

    styleData.addOption(Option(name, value));
  }

  return true;
}

bool
CCSS::
readId(CStrParse &parse, std::string &id) const
{
  id = "";

  parse.skipSpace();

  while (! parse.eof() && ! parse.isSpace() && ! parse.isOneOf("{,>+~")) {
    char c;

    parse.readChar(&c);

    id += c;
  }

  parse.skipSpace();

  return ! id.empty();
}

bool
CCSS::
readBracedString(CStrParse &parse, std::string &str) const
{
  str = "";

  parse.skipChar();

  parse.skipSpace();

  while (! parse.eof() && ! parse.isChar('}')) {
    char c;

    parse.readChar(&c);

    str += c;
  }

  if (! parse.isChar('}')) {
    if (isDebug())
      std::cerr << "Missing close brace : '" << parse.stateStr() << "'" << std::endl;
    return false;
  }

  parse.skipChar();

  parse.skipSpace();

  return true;
}

bool
CCSS::
skipComment(CStrParse &parse) const
{
  parse.skipChars(2);

  while (! parse.eof()) {
    if (parse.isString("*/")) {
      parse.skipChars(2);
      return true;
    }

    parse.skipChar();
  }

  if (isDebug())
    std::cerr << "Unterminated commend : '" << parse.stateStr() << "'" << std::endl;

  return false;
}

bool
CCSS::
hasStyleData() const
{
  return ! styleData_.empty();
}

CCSS::StyleData &
CCSS::
getStyleData(const Selector &selector)
{
  auto p = styleData_.find(selector);

  if (p == styleData_.end())
    p = styleData_.insert(p, StyleDataMap::value_type(selector, StyleData(selector)));

  StyleData &styleData = (*p).second;

  return styleData;
}

const CCSS::StyleData &
CCSS::
getStyleData(const Selector &selector) const
{
  auto p = styleData_.find(selector);

  assert(p != styleData_.end());

  const StyleData &styleData = (*p).second;

  return styleData;
}

void
CCSS::
clear()
{
  styleData_.clear();
}

void
CCSS::
print(std::ostream &os, bool cdata) const
{
  for (const auto &d : styleData_) {
    const StyleData &styleData = d.second;

    styleData.print(os, cdata);

    os << std::endl;
  }
}

//----------

void
CCSS::StyleData::
print(std::ostream &os, bool cdata) const
{
  if (! cdata) {
    os << "<style class=\"";

    int i = 0;

    for (const auto &part : selector_.parts()) {
      if (i > 0)
        os << " ";

      os << *part;

      ++i;
    }

    os << "\"";

    for (const auto &o : options_)
      os << " " << o.getName() << "=\"" << o.getValue() << "\"";

    os << "/>";
  }
  else {
    int i = 0;

    for (const auto &part : selector_.parts()) {
      if (i > 0)
        os << " ";

      os << *part;

      ++i;
    }

    os << " {";

    for (const auto &o : options_)
      os << " " << o.getName() << ":" << o.getValue() << ";";

    os << " }";
  }
}
