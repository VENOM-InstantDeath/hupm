#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <string.h>
#include <dirent.h>
#include <sqlite3.h>
#include <unistd.h>
#include <json.h>
#include <curl/curl.h>
using namespace std;

size_t strappnet(char* ptr, size_t x, size_t y, string *str) {
	str->append(ptr);
	return 0;
}

int checkdb();

int main(int argc, char **argv) {
	const char* operation = nullptr; /*install update refresh remove*/
	int options[3]; /*0.path 1.cache 2.no-confirm*/
	memset(options, 0, 3);
	if (argc < 2) {
		puts("\033[1;31mError\033[0m: No arguments provided");
		return 1;
	}
	/*This for checks command line arguments*/
	for (int i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
			cout << "hupm - HackingUtils Package Manager" << endl;
			cout << "Commands" << endl;
			cout << "\t(i)install\tInstall specified packages." << endl;
			cout << "\t(u)update\tUpdate some or all installed packages." << endl;
			cout << "\t(r)refresh\tRefresh hupm databases." << endl;
			cout << "\t(rm)remove\tRemove specified pkgs." << endl;
			cout << "Options" << endl;
			cout << "\t-p --path\tSpecify pkg's path (local installation)" << endl;
			cout << "\t-c --cache\tInstall/Search/Remove from cache." << endl;
			cout << "\t--noconfirm\tDon't ask user for confirmation." << endl;
			return 0;
		}
		else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--path")) {options[0] = 1;}
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--cache")) {options[1] = 1;}
		else if (!strcmp(argv[i], "--no-confirm")) {options[2] = 1;}
		else if (!strcmp(argv[i], "install") || !strcmp(argv[i], "i")) {
			if (operation==nullptr) {operation="install";} else {cout<<"\033[1;31mError\033[0m: More than one command specified.\n";}}
		else if (!strcmp(argv[i], "update") || !strcmp(argv[i], "u")) {
			if (operation==nullptr) {operation="update";} else {cout<<"\033[1;31mError\033[0m: More than one command specified.\n";}}
		else if (!strcmp(argv[i], "refresh") || !strcmp(argv[i], "r")) {
			if (operation==nullptr) {operation="refresh";} else {cout<<"\033[1;31mError\033[0m: More than one command specified.\n";}}
		else if (!strcmp(argv[i], "remove") || !strcmp(argv[i], "rm")) {
			if (operation==nullptr) {operation="remove";} else {cout<<"\033[1;31mError\033[0m: More than one command specified.\n";}}
		else {cout << "\033[1;31mError\033[0m: argument '" << argv[i] << "' is invalid."<<endl;return 1;}
	}
	/*Check for invalid set of options*/
	if (options[0] && options[1]) {
		puts("\033[1;31mError\033[0m: Invalid set of options");
	}
	/*start dealing with the operations*/
	if (operation == nullptr) {
		puts("\033[1;31mError\033[0m: No command specified");
		return 1;
	}
	if (operation == "refresh") {
		/* Check permissions */
		if (getuid()) {std::cout << "\033[1;31mError\033[0m: Insufficient permissions." << std::endl;return 1;}
		/*Read config*/
		std::ifstream conf("/var/lib/hupm/conf.json");
		std::stringstream jstr;
		jstr << conf.rdbuf();
		/* In a loop, we download each db*/
		json_object *jobj = json_tokener_parse(jstr.str().c_str());
		json_object_object_foreach(jobj, key, val) {
			json_object *kobj = json_object_object_get(jobj, key);
			std::string dbname = key; dbname += ".db";
			std::string url = json_object_get_string(kobj);
			url += dbname;
			/*download it now*/
			std::cout << "Downloading " << key <<"... ";
			FILE *fdb = fopen(dbname.c_str(), "w+");
			CURL *curl = curl_easy_init();
			CURLcode res;
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fdb);
			res = curl_easy_perform(curl);
			if (res == CURLE_OK) {
				std::cout << "\033[1;32mOK\033[0m\n";
			} else {
				std::cout << "\033[1;31mError\033[0m\n";
			}
			curl_easy_cleanup(curl);
		}
		/* file and in the same cycle we write it. (overwrite)*/
	}
	if (operation == "install");
	if (operation == "update");
	if (operation == "remove");
	/*CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION);*/
	return 0;
}
