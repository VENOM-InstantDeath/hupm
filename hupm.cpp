#include <iostream>
#include <string.h>
#include <algorithm>
#include <vector>
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
		cout << argc << endl;
		//argv[2]
		vector<string> insopts = {"--local", "-l"};
		int setting[] = {0}; //0-local
		if (argc < 3) {
			cout << "hupm: \033[1;31mno targets specified\033[0m\n";
			cout << "Try \033[1;32m`hupm --help`\033[0m for more information\n";
			return 1;
		}
		for (int i=2; i<argc; i++) {
			if (argv[i][0] == '-') {
				cout << "Opcion: " << argv[i] << endl;
				if (find(insopts.begin(), insopts.end(), argv[i]) == insopts.end()) {
					cout << "hupm: \033[1;31minvalid option: " << argv[i] << "\033[0m\n";
					return 1;
				}
			}
			if (argv[i] == "--local" || argv[i] == "-l") {
				cout << "Local\n";
				setting[0] = 1;
			}
		}
		for (int i=2; i<argc; i++) {
			if (! (argv[i][0] == '-')) {
				cout << argv[i] << endl;
				//directorio -- /var/lib/hupm/ packs-data
				if (!setting[0]) {
					curl = curl_easy_init();
					if (curl) {
						curl_easy_setopt(curl, CURLOPT_URL, "https://raw.githubusercontent.com/VENOM-InstantDeath/");
					}
				}
			}
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
		cout << "hupm: \033[1;31minvalid operation: '" << argv[1] << "'\033[0m" << endl;
		return 1;
	}

	return 0;
}
