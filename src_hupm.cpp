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
#include <sys/stat.h>
using namespace std;
int DEBUG = 0;

size_t strappnet(char* ptr, size_t x, size_t y, string *str) {
	str->append(ptr);
	return 0;
}

char* extract_version(char* pkgname) {
	char* str = (char*)calloc(strlen(pkgname), 1);
	int p=0;
	for (int i=strlen(pkgname)-1; i>=0; i--) {
		if (pkgname[i] == '-') break;
		str[p] = pkgname[i];p++;
	}
	str = (char*)realloc(str, strlen(str));
	char* ver = (char*)calloc(strlen(str), 1);
	p = 0;
	for (int i=strlen(str)-1; i>=0;i--) {ver[p] = str[i];p++;}
	free(str);
	return ver;
}

std::vector<const char*> vecsplit(const char* deps) {
	std::vector<const char*> vec;
	if (!strcmp(deps, "-")) return vec;
	char* x = (char*)calloc(40, 1);
	int c=0;
	for (int i=0; i<strlen(deps); i++) {
		if (deps[i] == ' ') {
			vec.push_back(x);
			x = (char*)calloc(40,1);c=0;
		}
		else {x[c] = deps[i];c++;}
	}
	vec.push_back(x);
	return vec;
}

void printvec(std::vector<const char*> s) {
	std::cout << "[";
	if (!s.size()) {std::cout << "]\n";return;}
	for (int i=0; i<s.size()-1; i++) {
		std::cout << s[i] << ", ";
	}
	std::cout << s[s.size()-1] << "]\n";
}

int vecchkelement(std::vector<const char*> li, const char* pkg) {
	for (int i=0; i<li.size(); i++) {
		if (!strcmp(li[i], pkg)) return 1;
	}
	return 0;
}

int vecindex(const char* pkg, std::vector<const char*> v) {
	for (int i=0; i<v.size(); i++) {
		if (!strcmp(pkg, v[i])) return i;
	}
	throw "out of bounds";
}

std::vector<const char*> extractdeps(const char* pkg) {
	struct dirent *dirst;
	std::string dbpath = "/var/lib/hupm/data/";
	DIR *dir = opendir(dbpath.c_str());
	int found=0;
	const char* result;
	while ((dirst = readdir(dir)) != NULL) {
		if (!strcmp(dirst->d_name, ".") && !strcmp(dirst->d_name, "..")) continue;
		sqlite3 *db;
		sqlite3_stmt *stmt;
		sqlite3_open((dbpath+dirst->d_name).c_str(), &db);
		int res;
		const char* sql = "SELECT * FROM pkgtab WHERE basename=?";
		sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
		sqlite3_bind_text(stmt, 1, pkg, -1, NULL);
		res = sqlite3_step(stmt);
		if (res == SQLITE_DONE) {sqlite3_finalize(stmt);sqlite3_close(db);continue;}
		else if (res == SQLITE_ROW) {
			result = (const char*)sqlite3_column_blob(stmt, 4);
			found=1;break;
		}
		sqlite3_finalize(stmt);sqlite3_close(db);
	}
	closedir(dir);
	if (!found) {
		throw "Package not found";
	} else {
		/*std::cout << "in extractdeps: pkg: " << pkg << std::endl;
		std::cout << "in extractdeps: result: " << "<" << result << ">"<< std::endl;*/
		std::vector<const char*> toret = vecsplit(result);
		/*std::cout << "in extractdeps: toret size: " << toret.size() << std::endl;
		printvec(toret);*/
		return toret;
	}
}

void solve(const char* pkg, std::vector<const char*> *li) {
	std::vector<const char*> res = extractdeps(pkg);
	/*printvec(res);*/
	if (!res.size()) {
		if (!vecchkelement(*li, pkg)) {
			li->push_back(pkg);
		}
	} else {
		for (int i=0; i<res.size();i++) {
			solve(res[i], li);
		}
		if (!vecchkelement(*li, pkg)) {
			li->push_back(pkg);
		}
	}
}

int fexist(const char* fname) {struct stat bf; return (stat(fname, &bf) == 0);}

int main(int argc, char **argv) {
	const char* operation = nullptr; /*install update refresh remove*/
	int options[3]; /*0.path 1.cache 2.no-confirm*/
	vector<char*> pkgreq;
	memset(options, 0, sizeof(options));
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
			cout << "\t-h --help\tShow help message and exit." << endl;
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
		else if (argv[i][0] == '-' || operation==nullptr){cout << "\033[1;31mError\033[0m: argument '" << argv[i] << "' is invalid."<<endl;return 1;}
		else {pkgreq.push_back(argv[i]);}
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
		/* In a loop, we download each db
		 * file and in the same cycle we write it.*/
		json_object *jobj = json_tokener_parse(jstr.str().c_str());
		json_object_object_foreach(jobj, key, val) {
			json_object *kobj = json_object_object_get(jobj, key);
			std::string dbname = key; dbname += ".db";
			std::string fdbname = "/var/lib/hupm/data/"; fdbname += dbname;
			std::string url = json_object_get_string(kobj);
			url += dbname;
			std::cout << "Downloading " << key <<"... ";
			FILE *fdb = fopen(fdbname.c_str(), "w+");
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
	}
	if (operation == "install") {
		if (getuid()) {std::cout << "\033[1;31mError\033[0m: Insufficient permissions." << std::endl;return 1;}
		if (!pkgreq.size()) {
			puts("\033[1;31mError\033[0m: No packages specified.");
			return 1;
		}
		/*Itera en /var/lib/hupm/db abriendo cada db
		 * y buscando si existe un paquete con el nombre
		 * especificado.*/
		struct dirent *dirst;
		std::string dbpath = "/var/lib/hupm/data/";
		for (int i=0; i<pkgreq.size(); i++) {
			DIR *dir = opendir(dbpath.c_str());
			int found=0;
			const char* version;
			const char* version2;
			std::vector<const char*> exdeps;
			//std::vector<const char*> deps;
			const char* deps;
			while ((dirst = readdir(dir)) != NULL) {
				if (!strcmp(dirst->d_name, ".") && !strcmp(dirst->d_name, "..")) continue;
				sqlite3 *db;
				sqlite3_stmt *stmt;
				sqlite3_open((dbpath+dirst->d_name).c_str(), &db);
				int res;
				const char* sql = "SELECT * FROM pkgtab WHERE basename=?";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
				sqlite3_bind_text(stmt, 1, argv[2], -1, NULL);
				res = sqlite3_step(stmt);
				if (res == SQLITE_DONE) {sqlite3_finalize(stmt);sqlite3_close(db);continue;}
				else if (res == SQLITE_ROW) {
					version = (const char*)sqlite3_column_blob(stmt, 6);
					exdeps = vecsplit((const char*)sqlite3_column_blob(stmt, 5));
					deps = (const char*)sqlite3_column_blob(stmt, 4);
					found=1;break;
				}
				sqlite3_finalize(stmt);sqlite3_close(db);
			}
			closedir(dir);
			if (!found) {
				std::cout << "\033[1;31mError\033[0m: Package not found: " << argv[2] << std::endl;
				return 1;
			} else std::cout << "Found pkg\n";
		/*Comprueba si el paquete está instalado. Si está
		 * instalado debe comprobar si es la misma versión
		 * que la de la db.*/
			/*Check if insdat.db exists*/
			found = 0;
			if (!fexist("/var/lib/hupm/pkgs/insdat.db")) {
				sqlite3 *db;
				sqlite3_open("/var/lib/hupm/pkgs/insdat.db", &db);
				sqlite3_stmt *stmt;
				const char *sql = "CREATE TABLE pkgtab (id INTEGER PRIMARY KEY AUTOINCREMENT, basename CHAR(40), pkgname CHAR(50), size INTEGER, deps TEXT, exdeps TEXT, version CHAR(15), author CHAR(25), desc TEXT);";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
				sqlite3_step(stmt);
				sqlite3_finalize(stmt);
				sqlite3_close(db);
			} else {
				sqlite3 *db;
				sqlite3_stmt *stmt;
				sqlite3_open("/var/lib/hupm/pkgs/insdat.db", &db);
				const char* sql = "SELECT * FROM pkgtab WHERE basename=?";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
				sqlite3_bind_text(stmt, 1, argv[2], -1, NULL);
				int res = sqlite3_step(stmt);
				if (res == SQLITE_ROW) {
					version2 = (const char*)sqlite3_column_blob(stmt, 6);
					found = 1;
				}
				sqlite3_finalize(stmt);
				sqlite3_close(db);
			}
			if (found) {
				puts("Package is already installed.");
				if (!strcmp(version, version2)) {
					/*Same version. Ask to reinstall*/
				} else {
					/*Different version. Ask to update*/
				}
			} else {
				puts("Package is not installed.");
				std::vector<const char*> li;
				solve(pkgreq[i], &li);
				printf("The following packages will be installed:\n");
				printf("  ");
				for (int i=0; i<li.size(); i++) {
					printf("%s ", li[i]);
				}
				printf("\n");
				/*Begin installation*/
				/*Solve dependencies*/
				/*Download packages*/
				/*Uncompress tarballs*/
				/*execute huscript*/
			}
		}
		/*Si no está instalado, se resuelven dependencias,
		 * se imprimen, se imprime el peso total y se con-
		 * sulta al usuario para proceder.*/

		/*Si está instalado, se consulta si se quiere actu-
		 * alizar.*/
	}
	if (operation == "update");
	if (operation == "remove");
	/*CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION);*/
	return 0;
}
