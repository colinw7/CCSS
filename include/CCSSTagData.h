#ifndef CCSSTagData_H
#define CCSSTagData_H

#include <vector>
#include <memory>

// Interface class to check if css selector matches tag data
class CCSSTagData;

typedef std::shared_ptr<CCSSTagData> CCSSTagDataP;

//---

enum class CCSSAttributeOp {
  NONE,
  EQUAL,
  PARTIAL,
  STARTS_WITH
};

//---

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

  virtual bool isNthChild(int n) const = 0;

  virtual bool isInputValue(const std::string &value) const = 0;

  virtual CCSSTagDataP getParent() const = 0;

  virtual void getChildren(TagDataArray &children) const = 0;

  virtual CCSSTagDataP getPrevSibling() const = 0;

  virtual CCSSTagDataP getNextSibling() const = 0;
};

#endif
