#include <iostream>

int main(int argc, char **argv)
{
  for (auto i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "-h") {
      std::cout << "Something help" << std::endl;
      return 1;
    }
  }

  return 0;
}
