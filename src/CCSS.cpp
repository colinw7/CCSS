#include <CCSS.h>
#include <CXML.h>
#include <CXMLParser.h>
#include <CFile.h>
#include <CStrUtil.h>
#include <CStrParse.h>
#include <CRegExp.h>
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
    errorMsg("Invalid file '" + filename + "'");
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

bool
CCSS::
parseSelector(const std::string &id, std::vector<StyleData> &styles)
{
  CStrParse parse(id);

  // get ids
  IdListList idListList;

  if (! parseIdListList(parse, idListList))
    return false;

  // add selector for each comma separated id
  for (const auto &idList : idListList) {
    SelectorList selectorList;

    // add part for each space separator or child operator separated id
    for (const auto &id : idList) {
      Selector selector;

      addSelectorParts(selector, id);

      selectorList.addSelector(selector);
    }

    StyleData &styleData1 = getStyleData(selectorList);

    styles.push_back(styleData1);
  }

  return true;
}

void
CCSS::
getSelectors(std::vector<SelectorList> &selectors) const
{
  for (const auto &d : styleData_) {
    const SelectorList &selectorList = d.first;

    selectors.push_back(selectorList);
  }
}

bool
CCSS::
parse(const std::string &str)
{
  CStrParse parse(str);

  while (! parse.eof()) {
    parse.skipSpace();

    if (isComment(parse)) {
      skipComment(parse);

      parse.skipSpace();
    }

    //---

    // get ids
    IdListList idListList;

    if (! parseIdListList(parse, idListList))
      return false;

    //---

    if (! parse.isChar('{')) {
      errorMsg("Missing '{' for rule");
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

    // add selector for each comma separated id
    for (const auto &idList : idListList) {
      SelectorList selectorList;

      // add part for each space separator or child operator separated id
      for (const auto &id : idList) {
        Selector selector;

        addSelectorParts(selector, id);

        selectorList.addSelector(selector);
      }

      StyleData &styleData1 = getStyleData(selectorList);

      for (const auto &opt : styleData.getOptions())
        styleData1.addOption(opt);
    }
  }

  return true;
}

bool
CCSS::
parseIdListList(CStrParse &parse, IdListList &idListList)
{
  // get ids

  while (! parse.eof()) {
    // read comma separated list of space separated ids
    while (! parse.eof()) {
      IdList idList;

      while (! parse.eof()) {
        Id id;

        // read selector id
        if (! readId(parse, id.id)) {
          errorMsg("Empty id : '" + parse.stateStr() + "'");
          return false;
        }

        // check for child selector
        if      (parse.isOneOf(">+~")) {
          if      (parse.isChar('>'))
            id.nextType = NextType::CHILD;
          else if (parse.isChar('+'))
            id.nextType = NextType::SIBLING;
          else if (parse.isChar('~'))
            id.nextType = NextType::PRECEDER;

          parse.skipChar();

          parse.skipSpace();

          idList.push_back(id);
        }
        // break if no more ids '}', or new set of ids ','
        else if (parse.isChar(',') || parse.isChar('{')) {
          idList.push_back(id);

          break;
        }
        else {
          id.nextType = NextType::DESCENDANT;

          idList.push_back(id);
        }
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

  return true;
}

void
CCSS::
addSelectorParts(Selector &selector, const Id &id)
{
  SelectorData selectorData;

  parseSelectorData(id.id, selectorData);

  //---

  selector.setName(selectorData.name);

  selector.setNextType(id.nextType);

  selector.setIdNames(selectorData.idNames);

  selector.setClassNames(selectorData.classNames);

  selector.setExpressions(selectorData.exprs);

  selector.setFunctions(selectorData.fns);
}

void
CCSS::
parseSelectorData(const std::string &id, SelectorData &selectorData) const
{
  assert(! id.empty());

  selectorData.name = id;

  int pos = 0;

  // <name> [# <id> ]
  if (findIdChar(selectorData.name, '#', pos)) {
    std::string idName = selectorData.name.substr(pos + 1);

    selectorData.name = selectorData.name.substr(0, pos);

    int pos1 = 0;

    while (findIdChar(idName, '#', pos1)) {
      std::string lhs = idName.substr(0, pos1);
      std::string rhs = idName.substr(pos1 + 1);

      selectorData.idNames.push_back(lhs);

      idName = rhs;
    }

    selectorData.idNames.push_back(idName);
  }

  // <name> [. <class> ]
  if (findIdChar(selectorData.name, '.', pos)) {
    std::string className = selectorData.name.substr(pos + 1);

    selectorData.name = selectorData.name.substr(0, pos);

    int pos1 = 0;

    while (findIdChar(className, '.', pos1)) {
      std::string lhs = className.substr(0, pos1);
      std::string rhs = className.substr(pos1 + 1);

      selectorData.classNames.push_back(lhs);

      className = rhs;
    }

    selectorData.classNames.push_back(className);
  }

  //---

  // <name> [ <expr> ]
  auto p = selectorData.name.find('[');

  if (p != std::string::npos) {
    std::string exprStr = selectorData.name.substr(p + 1);

    selectorData.name = selectorData.name.substr(0, p);

    auto p1 = exprStr.find(']');

    if (p1 != std::string::npos) {
      std::string rhs = exprStr.substr(p1 + 1);

      exprStr = exprStr.substr(0, p1);

      std::size_t i = 0;

      while (i < rhs.size() && isspace(rhs[i]))
        ++i;

      while (i < rhs.size() && rhs[i] == '[') {
        Expr expr(exprStr);

        selectorData.exprs.push_back(expr);

        ++i;

        rhs = rhs.substr(i);

        auto p2 = rhs.find(']');

        if (p2 != std::string::npos) {
          exprStr = rhs.substr(0, p2);
          rhs     = rhs.substr(p2 + 1);

          i = 0;
        }
      }

      selectorData.exprs.push_back(Expr(exprStr));
    }
  }

  //---

  // <name>:<fn> [:<fn>...]
  p = selectorData.name.find(':');

  if (p != std::string::npos) {
    std::string fn = selectorData.name.substr(p + 1);

    selectorData.name = selectorData.name.substr(0, p);

    auto p1 = fn.find(':');

    while (p1 != std::string::npos) {
      std::string lhs = fn.substr(0, p1);
      std::string rhs = fn.substr(p1 + 1);

      selectorData.fns.push_back(lhs);

      fn = rhs;

      p1 = fn.find(':');
    }

    selectorData.fns.push_back(fn);
  }
}

bool
CCSS::
findIdChar(const std::string &str, char c, int &pos)
{
  int i = 0;

  while (str[i] != '\0') {
    if      (str[i] == '[') {
      int sbracket = 1;

      while (str[i] != '\0') {
        if      (str[i] == '[')
          ++sbracket;
        else if (str[i] == ']') {
          --sbracket;

          if (sbracket == 0)
            break;
        }

        ++i;
      }
    }
    else if (str[i] == c) {
      pos = i;
      return true;
    }

    ++i;
  }

  return false;
}

bool
CCSS::
parseAttr(const std::string &str, StyleData &styleData)
{
  static std::string importantStr = "!important";

  CStrParse parse(str);

  parse.skipSpace();

  if (parse.eof())
    return true;

  while (! parse.eof()) {
    std::string name = readAttrName(parse);

    parse.skipSpace();

    // TODO: comma separated values
    // TODO: collapse white space
    std::string value;
    bool        important = false;

    if (parse.isChar(':')) {
      parse.skipChar();

      parse.skipSpace();

      value = readAttrValue(parse);

      if (value.size() >= importantStr.size()) {
        int rpos = value.size() - importantStr.size();

        if (value.substr(rpos) == importantStr) {
          important = true;

          value = value.substr(0, rpos);

          value = CStrUtil::stripSpaces(value);
        }
      }

      if (parse.isChar(';')) {
        parse.skipChar();

        parse.skipSpace();
      }
    }

    if (name.empty()) {
      errorMsg("Empty name : '" + parse.stateStr() + "'");
      return false;
    }

    styleData.addOption(Option(name, value, important));
  }

  return true;
}

std::string
CCSS::
readAttrName(CStrParse &parse) const
{
  std::string name;

  while (! parse.eof() && ! parse.isSpace() && ! parse.isChar(':')) {
    char c;

    parse.readChar(&c);

    name += c;
  }

  return name;
}

std::string
CCSS::
readAttrValue(CStrParse &parse) const
{
  std::string value;

  while (! parse.eof() && ! parse.isChar(';')) {
    char c;

    parse.readChar(&c);

    value += c;
  }

  value = CStrUtil::stripSpaces(value);

  return value;
}

bool
CCSS::
readId(CStrParse &parse, std::string &id) const
{
  id = "";

  parse.skipSpace();

  while (! parse.eof() && ! parse.isSpace() && ! parse.isOneOf("{,>+~")) {
    char c;

    if (parse.isChar('[')) {
      char c;

      parse.readChar(&c);

      id += c;

      int sbracket = 1;

      while (! parse.eof()) {
        parse.readChar(&c);

        id += c;

        if      (c == '[')
          ++sbracket;
        else if (c == ']') {
          --sbracket;

          if (sbracket == 0)
            break;
        }
      }
    }
    else {
      parse.readChar(&c);

      id += c;
    }
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
    if (isComment(parse))
      skipComment(parse);

    char c;

    parse.readChar(&c);

    str += c;
  }

  if (! parse.isChar('}')) {
    errorMsg("Missing close brace : '" + parse.stateStr() + "'");
    return false;
  }

  parse.skipChar();

  parse.skipSpace();

  return true;
}

bool
CCSS::
isComment(CStrParse &parse) const
{
  return parse.isString("/*");
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

  errorMsg("Unterminated commend : '" + parse.stateStr() + "'");

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
getStyleData(const SelectorList &selectorList)
{
  auto p = styleData_.find(selectorList);

  if (p == styleData_.end())
    p = styleData_.insert(p, StyleDataMap::value_type(selectorList, StyleData(selectorList)));

  StyleData &styleData = (*p).second;

  return styleData;
}

const CCSS::StyleData &
CCSS::
getStyleData(const SelectorList &selectorList) const
{
  auto p = styleData_.find(selectorList);

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
printStyle(std::ostream &os) const
{
  for (const auto &d : styleData_) {
    const StyleData &styleData = d.second;

    styleData.printStyle(os);

    os << std::endl;
  }
}

void
CCSS::
print(std::ostream &os) const
{
  for (const auto &d : styleData_) {
    const StyleData &styleData = d.second;

    if (isDebug())
      styleData.printDebug(os);
    else
      styleData.print(os);

    os << std::endl;
  }
}

void
CCSS::
errorMsg(const std::string &msg) const
{
  if (isDebug())
    std::cerr << msg << std::endl;
}

//----------

void
CCSS::Expr::
init(const std::string &str)
{
  CStrParse parse(str);

  //---

  // read id
  parse.skipSpace();

  char c;

  while (! parse.eof() && ! parse.isSpace() && ! parse.isOneOf("=~|")) {
    parse.readChar(&c);

    id_ += c;
  }

  //---

  // read op
  parse.skipSpace();

  if      (parse.isChar('=')) {
    parse.skipChar();

    op_ = CCSSAttributeOp::EQUAL;
  }
  else if (parse.isChar('~')) {
    parse.skipChar();

    if (parse.isChar('=')) {
      parse.skipChar();

      op_ = CCSSAttributeOp::PARTIAL;
    }
    else {
      // TODO: error
    }
  }
  else if (parse.isChar('|')) {
    parse.skipChar();

    if (parse.isChar('=')) {
      parse.skipChar();

      op_ = CCSSAttributeOp::STARTS_WITH;
    }
    else {
      // TODO: error
    }
  }

  //---

  // read value
  parse.skipSpace();

  if (parse.isChar('"')) {
    parse.skipChar();

    while (! parse.eof() && ! parse.isChar('"')) {
      parse.readChar(&c);

      value_ += c;
    }

    if (parse.isChar('"'))
      parse.skipChar();
  }
}

//----------

bool
CCSS::Selector::
checkMatch(const CCSSTagDataP &data) const
{
  // check name
  if (name_ != "" && name_ != "*") {
    if (! data->isElement(name_))
      return false;
  }

  //---

  // check ids
  if (! idNames_.empty()) {
    // must match all
    bool match = true;

    for (const auto &idName : idNames_) {
      if (! data->isId(idName)) {
        match = false;
        break;
      }
    }

    if (! match)
      return false;
  }

  //---

  // check classes
  if (! classNames_.empty()) {
    // must match all
    bool match = true;

    for (const auto &className : classNames_) {
      if (! data->isClass(className)) {
        match = false;
        break;
      }
    }

    if (! match)
      return false;
  }

  //---

  // check expressions
  if (! exprs_.empty()) {
    // must match all
    bool match = true;

    for (const auto &expr : exprs_) {
      if (! data->hasAttribute(expr.id(), expr.op(), expr.value())) {
        match = false;
        break;
      }
    }

    if (! match)
      return false;
  }

  //---

  // check functions
  if (! fns_.empty()) {
    // must match all
    bool match = true;

    bool error = false;

    for (const auto &fn : fns_) {
      std::vector<std::string> match_strs;

      if      (CRegExpUtil::parse(fn, "nth-child(\\(.*\\))", match_strs)) {
        long value;

        if (! CStrUtil::toInteger(match_strs[0], &value))
          continue;

        if (! data->isNthChild(value)) {
          match = false;
          break;
        }
      }
      else if (fn == "required" || fn == "invalid") {
        if (! data->isInputValue(fn)) {
          match = false;
          break;
        }
      }
      else {
        if (! error) {
          std::cerr << "selector functions not handled:";
          error = true;
        }

        std::cerr << " " << fn;
      }
    }

    if (error)
      std::cerr << std::endl;

    if (! match)
      return false;
  }

  //---

  return true;
}

//----------

bool
CCSS::SelectorList::
checkMatch(const CCSSTagDataP &data) const
{
  // all selectors must match
  for (const auto &selector : selectors_) {
    if (! selector.checkMatch(data))
      return false;
  }

  return true;
}

//----------

bool
CCSS::StyleData::
checkMatch(const CCSSTagDataP &data) const
{
  const auto &selectors = selectorList_.selectors();

  if (selectors.empty())
    return false;

  //---

  // match last selector
  std::size_t n = selectors.size();

  int i = n - 1;

  const CCSS::Selector &selector = selectors[i];

  if (! selector.checkMatch(data))
    return false;

  --i;

  //---

  if (i >= 0) {
    CCSSTagData::TagDataArray currentTagDatas;

    currentTagDatas.push_back(data);

    while (i >= 0) {
      const CCSS::Selector &selector = selectors[i];

      CCSSTagData::TagDataArray parentTagDatas;

      if      (selector.nextType() == NextType::DESCENDANT) {
        // collect list of any parents which match selector
        for (const auto &currentTagData : currentTagDatas) {
          CCSSTagDataP parent = currentTagData->getParent();

          while (parent) {
            if (selector.checkMatch(parent))
              parentTagDatas.push_back(parent);

            parent = parent->getParent();
          }
        }

        if (parentTagDatas.empty())
          return false;
      }
      else if (selector.nextType() == NextType::CHILD) {
        // update list if parent matches selector
        for (const auto &currentTagData : currentTagDatas) {
          CCSSTagDataP parent = currentTagData->getParent();
          if (! parent) continue;

          if (selector.checkMatch(parent))
            parentTagDatas.push_back(parent);
        }

        if (parentTagDatas.empty())
          return false;
      }
      else if (selector.nextType() == NextType::SIBLING) {
        // update list if previous sibling matches selector
        for (const auto &currentTagData : currentTagDatas) {
          CCSSTagDataP child = currentTagData->getPrevSibling();
          if (! child) continue;

          if (selector.checkMatch(child))
            parentTagDatas.push_back(child);
        }

        if (parentTagDatas.empty())
          return false;
      }
      else if (selector.nextType() == NextType::PRECEDER) {
        // update list if any previous sibling matches selector
        for (const auto &currentTagData : currentTagDatas) {
          CCSSTagDataP child = currentTagData->getPrevSibling();

          while (child) {
            if (selector.checkMatch(child))
              parentTagDatas.push_back(child);

            child = child->getPrevSibling();
          }
        }

        if (parentTagDatas.empty())
          return false;
      }

      currentTagDatas = parentTagDatas;

      --i;
    }
  }

  return true;
}

void
CCSS::StyleData::
printStyle(std::ostream &os) const
{
  os << "<style class=\"";

  int i = 0;

  for (const auto &selector : selectorList_.selectors()) {
    if (i > 0)
      os << " ";

    os << selector;

    ++i;
  }

  os << "\"";

  for (const auto &o : options_) {
    os << " ";

    o.printStyle(os);
  }

  os << "/>";
}

void
CCSS::StyleData::
print(std::ostream &os) const
{
  int i = 0;

  for (const auto &selector : selectorList_.selectors()) {
    if (i > 0)
      os << " ";

    os << selector;

    ++i;
  }

  os << " {";

  for (const auto &o : options_) {
    os << " ";

    o.print(os);
  }

  os << " }";
}

void
CCSS::StyleData::
printDebug(std::ostream &os) const
{
  int i = 0;

  for (const auto &selector : selectorList_.selectors()) {
    if (i > 0)
      os << " ";

    selector.printDebug(os);

    ++i;
  }

  //---

  i = 0;

  os << " {";

  for (const auto &o : options_) {
    if (i > 0)
      os << " ";

    o.printDebug(os);

    ++i;
  }

  os << "}";
}
