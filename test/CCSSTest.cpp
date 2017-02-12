#include <CCSS.h>
#include <cstring>

int
main(int argc, char **argv)
{
  std::string filename;

  bool debug       = false;
  bool style       = false;
  bool specificity = false;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if      (strcmp(&argv[i][1], "debug") == 0)
        debug = true;
      else if (strcmp(&argv[i][1], "style") == 0)
        style = true;
      else if (strcmp(&argv[i][1], "specificity") == 0)
        specificity = true;
      else
        std::cerr << "Invalid option: " << argv[i] << std::endl;
    }
    else
      filename = argv[i];
  }

  if (filename.empty())
    exit(1);

  CCSS css;

  css.setDebug(debug);

  css.processFile(filename);

  if (specificity) {
    std::vector<CCSS::SelectorList> selectorListArray;

    css.getSelectors(selectorListArray);

    for (const auto &selectorList : selectorListArray) {
      const CCSS::StyleData &styleData = css.getStyleData(selectorList);

      styleData.print(std::cout);

      std::cout << " [" << styleData.specificity() << "]";

      std::cout << std::endl;
    }
  }
  else if (style) {
    css.printStyle(std::cout);

    std::cout << std::endl;
  }
  else {
    css.print(std::cout);

    std::cout << std::endl;
  }

  return 0;
}
