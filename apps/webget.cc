#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main( int argc, char* argv[] )
{
  cout<<"argc is "<<argc<<endl;
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cout<<"args 0 是: "<<args[0]<<endl;
      cout<<"args 1 是: "<<args[1]<<endl;
      cout<<"args 2 是: "<<args[2]<<endl;
      cout<<"args 3 是: "<<args[3]<<endl;
      cout<< "zz coming!"<<endl;
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cout<<"出现错误了哟"<<endl;
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
