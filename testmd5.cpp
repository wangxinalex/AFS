 #include "md5.h"
 #include <iostream>
 
 using namespace std;
 
 void PrintMD5(const string& str, MD5& md5) {
      cout << "MD5(\"" << str << "\") = " << md5.toString() << endl;
 }

int main() {

     MD5 md5;

     md5.reset();
	 ifstream is("server_dir/test");
     md5.update(is);
     PrintMD5("server_dir/test", md5);
	 is.close();

     md5.reset();
	 is.open("client_dir/test");
     md5.update(is);
     PrintMD5("client_dir/test", md5);
	 is.close();
    return 0;
}
