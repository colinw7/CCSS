#include <CCSS.h>
#include <cstring>

int
main(int argc, char **argv)
{
  std::string filename;

  bool debug = false;

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(&argv[i][1], "debug") == 0)
        debug = true;
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

  std::cout << css << std::endl;
}
