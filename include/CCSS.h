#ifndef CCSS_H
#define CCSS_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sys/types.h>

class CStrParse;

class CCSS {
 public:
  class Option {
   public:
    Option(const std::string &name, const std::string &value) :
     name_(name), value_(value) {
    }

    const std::string &getName () const { return name_ ; }
    const std::string &getValue() const { return value_; }

   private:
    std::string name_;
    std::string value_;
  };

  typedef std::vector<Option> OptionList;

  //---

  class SelectPart {
   public:
    enum class Type {
      NONE,
      NAME,
      EXPR,
      FUNCTION
    };

   public:
    explicit SelectPart(Type type) :
     type_(type) {
    }

    SelectPart(const SelectPart &p) :
     type_(p.type_) {
    }

    virtual ~SelectPart() { }

    SelectPart &operator=(const SelectPart &p) {
      type_ = p.type_;

      return *this;
    }

    Type type() const { return type_; }

    virtual SelectPart *dup() const = 0;

    virtual void print(std::ostream &os) const = 0;

    virtual int cmp(const SelectPart &part) const = 0;

    friend bool operator<(const SelectPart &part1, const SelectPart &part2) {
      if (part1.type_ != part2.type_)
        return (part1.type_ < part2.type_);

      return part1.cmp(part2) < 0;
    }

    friend bool operator==(const SelectPart &part1, const SelectPart &part2) {
      if (part1.type_ != part2.type_)
        return false;

      return part1.cmp(part2) == 0;
    }

    friend std::ostream &operator<<(std::ostream &os, const SelectPart &data) {
      data.print(os);

      return os;
    }

   protected:
    Type type_;
  };

  //---

  class SelectName : public SelectPart {
   public:
    enum class Type {
      NONE,
      TYPE,
      CLASS,
      ID
    };

    enum class SubType {
      NONE,
      CHILD,
      SIBLING,
      PRECEDER
    };

   public:
    SelectName(Type type, const std::string &name, const std::string &className,
               SubType subType, const std::string &subName) :
     SelectPart(SelectPart::Type::NAME), type_(type), name_(name), className_(className),
     subType_(subType), subName_(subName) {
    }

    SelectName(const SelectName &n) :
     SelectPart(n), type_(n.type_), name_(n.name_), className_(n.className_),
     subType_(n.subType_), subName_(n.subName_) {
    }

    SelectName &operator=(const SelectName &n) {
      type_      = n.type_;
      name_      = n.name_;
      className_ = n.className_;
      subType_   = n.subType_;
      subName_   = n.subName_;

      return *this;
    }

    const Type &type() const { return type_; }

    const std::string &name() const { return name_; }

    const std::string &className() const { return className_; }

    const SubType &subType() const { return subType_; }

    const std::string &subName() const { return subName_; }

    SelectPart *dup() const override {
      return new SelectName(*this);
    }

    int cmp(const SelectPart &part) const override {
      const SelectName &other = dynamic_cast<const SelectName &>(part);

      if (type() < other.type()) return -1;
      if (type() > other.type()) return  1;

      if (name() < other.name()) return -1;
      if (name() > other.name()) return  1;

      if (className() < other.className()) return -1;
      if (className() > other.className()) return  1;

      if (subType() < other.subType()) return -1;
      if (subType() > other.subType()) return  1;

      if (subName() < other.subName()) return -1;
      if (subName() > other.subName()) return  1;

      return 0;
    }

    void print(std::ostream &os) const override {
      if      (type() == Type::ID)
        os << "#" << name();
      else if (type() == Type::CLASS)
        os << name() << "." << className();
      else
        os << name();

      if      (subType() == SubType::CHILD)
        os << " < " << subName();
      else if (subType() == SubType::SIBLING)
        os << " + " << subName();
      else if (subType() == SubType::PRECEDER)
        os << " ~ " << subName();
    }

   private:
    Type        type_ { Type::NONE };
    std::string name_;
    std::string className_;
    SubType     subType_ { SubType::NONE };
    std::string subName_;
  };

  //---

  class SelectExpr : public SelectPart {
   public:
    explicit SelectExpr(const std::string &expr) :
     SelectPart(SelectPart::Type::EXPR), expr_(expr) {
    }

    SelectExpr(const SelectExpr &i) :
     SelectPart(i), expr_(i.expr_) {
    }

    SelectExpr &operator=(const SelectExpr &i) {
      expr_ = i.expr_;

      return *this;
    }

    const std::string &expr() const { return expr_; }

    SelectPart *dup() const override {
      return new SelectExpr(*this);
    }

    int cmp(const SelectPart &part) const override {
      const SelectExpr &expr = dynamic_cast<const SelectExpr &>(part);

      if (expr_ < expr.expr_) return -1;
      if (expr_ > expr.expr_) return  1;

      return 0;
    }

    void print(std::ostream &os) const override {
      os << "[" << expr_ << "]";
    }

   private:
    std::string expr_;
  };

  //---

  class SelectFunction : public SelectPart {
   public:
    explicit SelectFunction(const std::string &fn) :
     SelectPart(SelectPart::Type::FUNCTION), fn_(fn) {
    }

    SelectFunction(const SelectFunction &i) :
     SelectPart(i), fn_(i.fn_) {
    }

    SelectFunction &operator=(const SelectFunction &i) {
      fn_ = i.fn_;

      return *this;
    }

    const std::string &fn() const { return fn_; }

    SelectPart *dup() const override {
      return new SelectFunction(*this);
    }

    int cmp(const SelectPart &part) const override {
      const SelectFunction &fn = dynamic_cast<const SelectFunction &>(part);

      if (fn_ < fn.fn_) return -1;
      if (fn_ > fn.fn_) return  1;

      return 0;
    }

    void print(std::ostream &os) const override {
      os << ":" << fn_;
    }

   private:
    std::string fn_;
  };

  //---

  class Selector {
   public:
    typedef std::vector<SelectPart *> Parts;

   public:
    Selector() { }

    Selector(const Selector &s) {
      for (auto &p : s.parts_)
        parts_.push_back(p->dup());
    }

   ~Selector() {
      for (auto &p : parts_)
        delete p;
    }

    Selector &operator=(const Selector &s) {
      for (auto &p : parts_)
        delete p;

      parts_.clear();

      for (auto &p : s.parts_)
        parts_.push_back(p->dup());

      return *this;
    }

    void addName(SelectName::Type type, const std::string &name, const std::string &className,
                 SelectName::SubType subType, const std::string &subName) {
      parts_.push_back(new SelectName(type, name, className, subType, subName));
    }

    void addExpr(const std::string &expr) {
      parts_.push_back(new SelectExpr(expr));
    }

    void addFunction(const std::string &fn) {
      parts_.push_back(new SelectFunction(fn));
    }

    const Parts &parts() const { return parts_; }

    SelectName::Type type() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::NAME)
          return dynamic_cast<const SelectName *>(p)->type();

      return SelectName::Type::NONE;
    }

    std::string name() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::NAME)
          return dynamic_cast<const SelectName *>(p)->name();

      return "";
    }

    std::string className() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::NAME)
          return dynamic_cast<const SelectName *>(p)->className();

      return "";
    }

    SelectName::SubType subType() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::NAME)
          return dynamic_cast<const SelectName *>(p)->subType();

      return SelectName::SubType::NONE;
    }

    std::string subName() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::NAME)
          return dynamic_cast<const SelectName *>(p)->subName();

      return "";
    }

    std::string expr() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::EXPR)
          return dynamic_cast<const SelectExpr *>(p)->expr();

      return "";
    }

    std::string function() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::FUNCTION)
          return dynamic_cast<const SelectFunction *>(p)->fn();

      return "";
    }

    friend bool operator<(const Selector &s1, const Selector &s2) {
      if (s1.parts_.size() != s2.parts_.size())
        return (s1.parts_.size() < s2.parts_.size());

      for (size_t i = 0; i < s1.parts_.size(); ++i) {
        if (! (*s1.parts_[i] == *s2.parts_[i]))
          return (*s1.parts_[i] < *s2.parts_[i]);
      }

      return false;
    }

    friend bool operator==(const Selector &s1, const Selector &s2) {
      if (s1.parts_.size() != s2.parts_.size())
        return false;

      for (size_t i = 0; i < s1.parts_.size(); ++i) {
        if (! (*s1.parts_[i] == *s2.parts_[i]))
          return false;
      }

      return true;
    }

   private:
    Parts parts_;
  };

  //---

  class StyleData {
   public:
    explicit StyleData(const Selector &selector=Selector()) :
     selector_(selector), options_() {
    }

    const Selector &getSelector() const { return selector_; }

    const OptionList &getOptions() const { return options_; }

    uint getNumOptions() const { return options_.size(); }

    const Option &getOption(uint i) const { return options_[i]; }

    void addOption(const Option &opt) { options_.push_back(opt); }

    friend std::ostream &operator<<(std::ostream &os, const StyleData &data) {
      data.print(os);

      return os;
    }

    void print(std::ostream &os, bool cdata=true) const;

   private:
    Selector   selector_;
    OptionList options_;
  };

  typedef std::map<Selector, StyleData> StyleDataMap;

  //---

 public:
  CCSS();

  bool isDebug() const { return debug_; }
  void setDebug(bool b) { debug_ = b; }

  bool processFile(const std::string &fileName);

  bool processLine(const std::string &line);

  void getSelectors(std::vector<Selector> &selectors) const;

  bool hasStyleData() const;

  StyleData &getStyleData(const Selector &selector);

  const StyleData &getStyleData(const Selector &selector) const;

  void clear();

  void print(std::ostream &os, bool cdata=true) const;

  friend std::ostream &operator<<(std::ostream &os, const CCSS &css) {
    css.print(os);

    return os;
  }

 private:
  bool parse(const std::string &str);
  bool parseAttr(const std::string &str, StyleData &styleData);

  bool readId(CStrParse &parse, std::string &id) const;

  bool readBracedString(CStrParse &parse, std::string &str) const;

  bool skipComment(CStrParse &parse) const;

  void addSelector(Selector &selector, const std::string &id,
                   SelectName::SubType subType, const std::string &subName);

 private:
  bool         debug_ { false };
  StyleDataMap styleData_;
};

#endif
