module;

#include <iostream>

export module server;

export namespace server {
  /// Bean server
  class Server
  {
    float number = 8.09F;

  public:
    void hello()
    {
      number += 2;
      std::cout << "nexoPVpZod :: Hello from Server: number=" << number << std::endl;
    }

    ~Server()
    {
      std::cout << "ZZzYOtuBsD :: Server are destroying..." << std::endl;
    }
  };
} // namespace server
