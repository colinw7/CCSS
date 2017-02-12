#ifndef CCSS_H
#define CCSS_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <sys/types.h>

class CStrParse;

//------

enum class CCSSAttributeOp {
  NONE,
  EQUAL,
  PARTIAL,
  STARTS_WITH
};

//------

// Interface class to check if css selector matches tage data
class CCSSTagData;

typedef std::shared_ptr<CCSSTagData> CCSSTagDataP;

class CCSSTagData {
 public:
  typedef std::vector<CCSSTagDataP> TagDataArray;

 public:
  CCSSTagData() { }

  virtual ~CCSSTagData() { }

  virtual bool isElement(const std::string &name) const = 0;

  virtual bool isClass(const std::string &name) const = 0;

  virtual bool isId(const std::string &name) const = 0;

  virtual bool hasAttribute(const std::string &name, CCSSAttributeOp op,
                            const std::string &value) const = 0;

  virtual CCSSTagDataP getParent() const = 0;

  virtual void getChildren(TagDataArray &children) const = 0;

  virtual CCSSTagDataP getPrevSibling() const = 0;

  virtual CCSSTagDataP getNextSibling() const = 0;
};

//------

class CCSS {
 public:
  // associated name type
  //  DESCENDANT : ' ' <name> , associated name is any child of selector
  //  CHILD      : '>' <name> , associated name is direct child of selector
  //  SIBLING    : '+' <name> , associated name is sibling of selector
  //  PRECEDER   : '~' <name> , associated name is preceder of selector
  enum class NextType {
    NONE,
    DESCENDANT,
    CHILD,
    SIBLING,
    PRECEDER
  };

  typedef std::vector<std::string> Names;

  //---

  // specificity of selector
  class Specificity {
   public:
    Specificity() { }

    void addId(int n=1) { value_[1] += n; }

    void addClass(int n=1) { value_[2] += n; }

    void addElement(int n=1) { value_[3] += n; }

    int cmp(const Specificity &s) const {
      for (int i = 0; i < 4; ++i) {
        if (value_[i] < s.value_[i]) return -1;
        if (value_[i] > s.value_[i]) return  1;
      }

      return 0;
    }

    Specificity &operator+=(const Specificity &s) {
      for (int i = 0; i < 4; ++i)
        value_[i] += s.value_[i];

      return *this;
    }

    friend bool operator<(const Specificity &s1, const Specificity &s2) {
      return s1.cmp(s2) < 0;
    }

    void print(std::ostream &os) const {
      for (int i = 0; i < 4; ++i) {
        if (i > 0) os << ",";

        os << value_[i];
      }
    }

    friend std::ostream &operator<<(std::ostream &os, const Specificity &s) {
      s.print(os);

      return os;
    }

   private:
    int value_[4] { 0, 0, 0,  0 };
  };

  //---

  // Selector id (name and next type)
  struct Id {
    std::string id;
    NextType    nextType { NextType::NONE };

    int cmp(const Id &rhs) const {
      if (id < rhs.id) return -1;
      if (id > rhs.id) return  1;

      if (nextType < rhs.nextType) return -1;
      if (nextType > rhs.nextType) return  1;

      return 0;
    }

    friend bool operator<(const Id &id1, const Id &id2) {
      return id1.cmp(id2) < 0;
    }

    friend bool operator==(const Id &id1, const Id &id2) {
      return id1.cmp(id2) == 0;
    }
  };

  typedef std::vector<Id>     IdList;
  typedef std::vector<IdList> IdListList;

  //---

  class Expr {
   public:
    explicit Expr(const std::string &str) {
      init(str);
    }

    void init(const std::string &str);

    const std::string &id() const { return id_; }

    const CCSSAttributeOp &op() const { return op_; }

    const std::string &value() const { return value_; }

    int cmp(const Expr &e) const {
      if (id_ < e.id_) return -1;
      if (id_ > e.id_) return  1;

      if (op_ < e.op_) return -1;
      if (op_ > e.op_) return  1;

      if (value_ < e.value_) return -1;
      if (value_ > e.value_) return  1;

      return 0;
    }

    friend bool operator<(const Expr &expr1, const Expr &expr2) {
      return expr1.cmp(expr2) < 0;
    }

    friend bool operator>(const Expr &expr1, const Expr &expr2) {
      return expr1.cmp(expr2) > 0;
    }

    friend bool operator==(const Expr &expr1, const Expr &expr2) {
      return expr1.cmp(expr2) == 0;
    }

    void print(std::ostream &os) const {
      os << id_;

      if      (op_ == CCSSAttributeOp::EQUAL)
        os << "=";
      else if (op_ == CCSSAttributeOp::PARTIAL)
        os << "~=";
      else if (op_ == CCSSAttributeOp::STARTS_WITH)
        os << "|=";

      if (value_ != "")
        os << "\"" << value_ << "\"";
    }

    friend std::ostream &operator<<(std::ostream &os, const Expr &expr) {
      expr.print(os);

      return os;
    }

   private:
    std::string     id_;
    CCSSAttributeOp op_ { CCSSAttributeOp::NONE };
    std::string     value_;
  };

  typedef std::vector<Expr> Exprs;

  //---

  // data for parse of id into type, class names, expressions and functions
  struct SelectorData {
    std::string name;
    Names       idNames;
    Names       classNames;
    Exprs       exprs;
    Names       fns;
  };

  //---

  // option (name/value)
  class Option {
   public:
    Option(const std::string &name, const std::string &value, bool important=false) :
     name_(name), value_(value), important_(important) {
    }

    const std::string &getName () const { return name_ ; }
    const std::string &getValue() const { return value_; }

    void printStyle(std::ostream &os) const {
      os << name_ << "=\"" << value_;

      if (important_)
        os << " !important";

      os << "\"";
    }

    void print(std::ostream &os) const {
      os << name_ << ": " << value_;

      if (important_)
        os << " !important";

      os << ";";
    }

    void printDebug(std::ostream &os) const {
      os << "{Name:" << name_ << "} {Value:" << value_ << "}";

      if (important_)
        os << " {!Important}";
    }

    friend std::ostream &operator<<(std::ostream &os, const Option &option) {
      option.print(os);

      return os;
    }

   private:
    std::string name_;
    std::string value_;
    bool        important_ { false };
  };

  typedef std::vector<Option> OptionList;

  //---

  // selector (name and optional expression, function parts)
  class Selector {
   public:
    Selector() { }

    const std::string &name() const { return name_; }
    void setName(const std::string &v) { name_ = v; }

    const Names &idNames() const { return idNames_; }
    void setIdNames(const Names &v) { idNames_ = v; }

    const Names &classNames() const { return classNames_; }
    void setClassNames(const Names &v) { classNames_ = v; }

    const Exprs &expressions() const { return exprs_; }
    void setExpressions(const Exprs &v) { exprs_ = v; }

    const Names &functions() const { return fns_; }
    void setFunctions(const Names &v) { fns_ = v; }

    const NextType &nextType() const { return nextType_; }
    void setNextType(const NextType &v) { nextType_ = v; }

    Specificity specificity() const {
      Specificity s;

      if (name_ != "" && name_ != "*")
        s.addElement();

      s.addId   (idNames   ().size());
      s.addClass(classNames().size());

      s.addClass(exprs_.size());

      s.addClass(fns_.size());

      return s;
    }

    bool checkMatch(const CCSSTagDataP &data) const;

    int cmp(const Selector &selector) const {
      if (name() < selector.name()) return -1;
      if (name() > selector.name()) return  1;

      //---

      if (idNames_.size() != selector.idNames_.size()) {
        if (idNames_.size() < selector.idNames_.size()) return -1;
        if (idNames_.size() > selector.idNames_.size()) return  1;
      }
      else {
        for (std::size_t i = 0; i < idNames_.size(); ++i) {
          if (idNames_[i] < selector.idNames_[i]) return -1;
          if (idNames_[i] > selector.idNames_[i]) return  1;
        }
      }

      //---

      if (classNames_.size() != selector.classNames_.size()) {
        if (classNames_.size() < selector.classNames_.size()) return -1;
        if (classNames_.size() > selector.classNames_.size()) return  1;
      }
      else {
        for (std::size_t i = 0; i < classNames_.size(); ++i) {
          if (classNames_[i] < selector.classNames_[i]) return -1;
          if (classNames_[i] > selector.classNames_[i]) return  1;
        }
      }

      //---

      if (exprs_.size() != selector.exprs_.size()) {
        if (exprs_.size() < selector.exprs_.size()) return -1;
        if (exprs_.size() > selector.exprs_.size()) return  1;
      }
      else {
        for (std::size_t i = 0; i < exprs_.size(); ++i) {
          if (exprs_[i] < selector.exprs_[i]) return -1;
          if (exprs_[i] > selector.exprs_[i]) return  1;
        }
      }

      //---

      if (fns_.size() != selector.fns_.size()) {
        if (fns_.size() < selector.fns_.size()) return -1;
        if (fns_.size() < selector.fns_.size()) return  1;
      }
      else {
        for (std::size_t i = 0; i < fns_.size(); ++i) {
          if (fns_[i] < selector.fns_[i]) return -1;
          if (fns_[i] > selector.fns_[i]) return  1;
        }
      }

      //---

      if (nextType() < selector.nextType()) return -1;
      if (nextType() > selector.nextType()) return  1;

      //---

      return 0;
    }

    friend bool operator<(const Selector &s1, const Selector &s2) {
      return s1.cmp(s2) < 0;
    }

    friend bool operator>(const Selector &s1, const Selector &s2) {
      return s1.cmp(s2) > 0;
    }

    friend bool operator==(const Selector &s1, const Selector &s2) {
      return s1.cmp(s2) == 0;
    }

    std::string toString() const {
      std::ostringstream ss;

      ss << *this;

      return ss.str();
    }

    void print(std::ostream &os) const {
      os << name();

      for (const auto &idName : idNames()) {
        os << "#" << idName;
      }

      for (const auto &className : classNames())
        os << "." << className;

      for (const auto &expr : exprs_)
        os << "[" << expr << "]";

      for (const auto &fn : fns_)
        os << ":" << fn;

      if      (nextType_ == NextType::CHILD)
        os << " >";
      else if (nextType_ == NextType::SIBLING)
        os << " +";
      else if (nextType_ == NextType::PRECEDER)
        os << " ~";
    }

    void printDebug(std::ostream &os) const {
      os << "{";

      os << "{Name:" << name() << "}";

      for (const auto &idName : idNames())
        os << " {Id:" << idName << "}";

      for (const auto &className : classNames())
        os << " {Class:" << className << "}";

      os << "}";

      if (! exprs_.empty()) {
        os << "{Expr:";

        for (const auto &expr : exprs_) {
          os << "[" << expr << "]";
        }

        os << "}";
      }

      if (! fns_.empty()) {
        os << "{Func:";

        for (const auto &fn : fns_)
          os << ":" << fn;

        os << "}";
      }

      if      (nextType_ == NextType::CHILD)
        os << " Child{>}";
      else if (nextType_ == NextType::SIBLING)
        os << " Sibling{+}";
      else if (nextType_ == NextType::PRECEDER)
        os << " Preceder{~}";
    }

    friend std::ostream &operator<<(std::ostream &os, const Selector &selector) {
      selector.print(os);

      return os;
    }

   private:
    std::string name_;                        // tag name
    Names       idNames_;                     // id names
    Names       classNames_;                  // class name
    Exprs       exprs_;                       // expressions
    Names       fns_;                         // functions
    NextType    nextType_ { NextType::NONE }; // next selector type
  };

  //---

  // selector list
  class SelectorList {
   public:
    typedef std::vector<Selector> Selectors;

   public:
    SelectorList() { }

    const Selectors &selectors() const { return selectors_; }

    void addSelector(const Selector &selector) {
      selectors_.push_back(selector);
    }

    Specificity specificity() const {
      Specificity s;

      for (const auto &selector : selectors_)
        s+= selector.specificity();

      return s;
    }

    bool checkMatch(const CCSSTagDataP &data) const;

    int cmp(const SelectorList &selectorList) const {
      if (selectors_.size() < selectorList.selectors_.size()) return -1;
      if (selectors_.size() > selectorList.selectors_.size()) return  1;

      for (size_t i = 0; i < selectors_.size(); ++i) {
        if (selectors_[i] < selectorList.selectors_[i]) return -1;
        if (selectors_[i] > selectorList.selectors_[i]) return  1;
      }

      return 0;
    }

    friend bool operator<(const SelectorList &s1, const SelectorList &s2) {
      return s1.cmp(s2) < 0;
    }

    friend bool operator==(const SelectorList &s1, const SelectorList &s2) {
      return s1.cmp(s2) == 0;
    }

    std::string toString() const {
      std::string str;

      for (const auto &selector : selectors_) {
        if (str != "") str += " ";

        str += selector.toString();
      }

      return str;
    }

   private:
    Selectors selectors_;
  };

  //---

  // style data (selector list and options)
  class StyleData {
   public:
    explicit StyleData(const SelectorList &selectorList=SelectorList()) :
     selectorList_(selectorList), options_() {
    }

    const SelectorList &getSelectorList() const { return selectorList_; }

    const OptionList &getOptions() const { return options_; }

    uint getNumOptions() const { return options_.size(); }

    const Option &getOption(uint i) const { return options_[i]; }

    void addOption(const Option &opt) { options_.push_back(opt); }

    bool getOptionValue(const std::string &name, std::string &value) const {
      for (const auto &option : options_) {
        if (option.getName() == name) {
          value = option.getValue();
          return true;
        }
      }

      return false;
    }

    Specificity specificity() const {
      return selectorList_.specificity();
    }

    bool checkMatch(const CCSSTagDataP &data) const;

    friend std::ostream &operator<<(std::ostream &os, const StyleData &data) {
      data.print(os);

      return os;
    }

    std::string toString() const {
      return selectorList_.toString();
    }

    void print(std::ostream &os) const;

    void printStyle(std::ostream &os) const;

    void printDebug(std::ostream &os) const;

   private:
    SelectorList selectorList_;
    OptionList   options_;
  };

  typedef std::map<SelectorList, StyleData> StyleDataMap;

  //---

 public:
  CCSS();

  bool isDebug() const { return debug_; }
  void setDebug(bool b) { debug_ = b; }

  bool processFile(const std::string &fileName);

  bool processLine(const std::string &line);

  void getSelectors(std::vector<SelectorList> &selectors) const;

  bool hasStyleData() const;

  StyleData &getStyleData(const SelectorList &selectorList);

  const StyleData &getStyleData(const SelectorList &selectorList) const;

  void clear();

  void printStyle(std::ostream &os) const;

  void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CCSS &css) {
    css.print(os);

    return os;
  }

 private:
  bool parse(const std::string &str);

  bool parseAttr(const std::string &str, StyleData &styleData);

  std::string readAttrName(CStrParse &parse) const;

  std::string readAttrValue(CStrParse &parse) const;

  static bool findIdChar(const std::string &str, char c, int &pos);

  bool readId(CStrParse &parse, std::string &id) const;

  bool readBracedString(CStrParse &parse, std::string &str) const;

  bool isComment(CStrParse &parse) const;

  bool skipComment(CStrParse &parse) const;

  void addSelectorParts(Selector &selector, const Id &id);

  void parseSelectorData(const std::string &id, SelectorData &selectorData) const;

  void errorMsg(const std::string &msg) const;

 private:
  bool         debug_ { false };
  StyleDataMap styleData_;
};

#endif
