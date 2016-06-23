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
      NAME,
      ID
    };

   public:
    SelectPart(Type type) :
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
    SelectName(const std::string &name) :
     SelectPart(SelectPart::Type::NAME), name_(name) {
    }

    SelectName(const SelectName &n) :
     SelectPart(n), name_(n.name_) {
    }

    SelectName &operator=(const SelectName &n) {
      name_ = n.name_;

      return *this;
    }

    const std::string &name() const { return name_; }

    SelectPart *dup() const override {
      return new SelectName(*this);
    }

    int cmp(const SelectPart &part) const override {
      const SelectName &name = dynamic_cast<const SelectName &>(part);

      if (name_ < name.name_) return -1;
      if (name_ > name.name_) return  1;

      return 0;
    }

    void print(std::ostream &os) const override {
      os << name_;
    }

   private:
    std::string name_;
  };

  //---

  class SelectId : public SelectPart {
   public:
    enum class Id {
      FIRST_CHILD,
      CHILD,
      SIBLING
    };

   public:
    SelectId(Id id) :
     SelectPart(SelectPart::Type::ID), id_(id) {
    }

    SelectId(const SelectId &i) :
     SelectPart(i), id_(i.id_) {
    }

    SelectId &operator=(const SelectId &i) {
      id_ = i.id_;

      return *this;
    }

    const Id &id() const { return id_; }

    SelectPart *dup() const override {
      return new SelectId(*this);
    }

    int cmp(const SelectPart &part) const override {
      const SelectId &id = dynamic_cast<const SelectId &>(part);

      if (id_ < id.id_) return -1;
      if (id_ > id.id_) return  1;

      return 0;
    }

    void print(std::ostream &os) const override {
      if      (id_ == Id::FIRST_CHILD) os << ":first-child";
      if      (id_ == Id::CHILD      ) os << ">";
      else if (id_ == Id::SIBLING    ) os << "+";
    }

   private:
    Id id_;
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

    const Selector &operator=(const Selector &s) {
      for (auto &p : parts_)
        delete p;

      parts_.clear();

      for (auto &p : s.parts_)
        parts_.push_back(p->dup());

      return *this;
    }

    void addName(const std::string &name) {
      parts_.push_back(new SelectName(name));
    }

    void addFirstChild() {
      parts_.push_back(new SelectId(SelectId::Id::FIRST_CHILD));
    }

    void addChild() {
      parts_.push_back(new SelectId(SelectId::Id::CHILD));
    }

    void addSibling() {
      parts_.push_back(new SelectId(SelectId::Id::SIBLING));
    }

    const Parts &parts() const { return parts_; }

    std::string id() const {
      for (auto &p : parts_)
        if (p->type() == SelectPart::Type::NAME)
          return dynamic_cast<const SelectName *>(p)->name();

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
    StyleData(const Selector &selector) :
     selector_(selector), options_() {
    }

    const Selector &getSelector() const { return selector_; }

    const OptionList &getOptions() const { return options_; }

    uint getNumOptions() const { return options_.size(); }

    const Option &getOption(uint i) const { return options_[i]; }

    void addOption(const std::string &name, const std::string &value) {
      options_.push_back(Option(name, value));
    }

    friend std::ostream &operator<<(std::ostream &os, const StyleData &data) {
      data.print(os);

      return os;
    }

    void print(std::ostream &os) const;

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

  StyleData &getStyleData(const Selector &selector);

  const StyleData &getStyleData(const Selector &selector) const;

  void print(std::ostream &os) const;

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

 private:
  bool         debug_ { false };
  StyleDataMap styleData_;
};

#endif
