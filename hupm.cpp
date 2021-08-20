#include <iostream>
#include <string.h>
#include <curl/curl.h>
using namespace std;

/* hupm - Hacking-Utils Package Manager
 *
 * options:
 *
 * hupm i[nstall] <package>
 * hupm r[emove]  <package>
 * hupm u[pdate]  <package>
 * hupm s[earch]  <package>
 * hupm a[bout]   <package>
 * hupm -h --help
 *
 * no args:
 *
 * hupm: no operation specified
 *
 * Try `hupm --help` for more information
 */

int main(int argc, char *argv[]) {
	if (argc == 1) {
		cout << "hupm: \033[1;31mno operation specified\033[0m\n";
		cout << "Try \033[1;32m`hupm --help`\033[0m for more information\n";
		return 1;
	}
	if (!strcmp(argv[1], "i") || !strcmp(argv[1], "install")) {
		cout << "Install\n";
		//argv[2]
		if (argc < 3) {
			cout << "hupm: \033[1;31mno targets specified\033[0m\n";
			cout << "Try \033[1;32m`hupm --help`\033[0m for more information\n";
			return 1;
		}
	} else if (!strcmp(argv[1], "r") || !strcmp(argv[1], "remove")) {
		cout << "Remove\n";
	} else if (!strcmp(argv[1], "u") || !strcmp(argv[1], "update")) {
		cout << "Update\n";
	} else if (!strcmp(argv[1], "s") || !strcmp(argv[1], "search")) {
		cout << "Search\n";
	} else if (!strcmp(argv[1], "a") || !strcmp(argv[1], "about")) {
		cout << "About\n";
	} else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		cout << "Help\n";
	} else {
		cout << "hupm: \033[1;31minvalid option '" << argv[1] << "'\033[0m" << endl;
		return 1;
	}

	return 0;
}
