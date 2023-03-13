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

struct pkgstruct {
	std::vector<const char*> pkg;
	std::vector<const char*> db;
};

size_t strappnet(char* ptr, size_t x, size_t y, string *str) {
	str->append(ptr);
	return 0;
}

double sizesum(std::vector<int> vec) {
	double size = 0;
	for (int i=0; i<vec.size(); i++) {
		size += vec[i];
	}
	size /= 1000;
	return size;
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

void printvec(std::vector<int> s) {
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

std::vector<const void*> extractfields(const char* pkg, char* fields, int nf) {
	struct dirent *dirst;
	std::string dbpath = "/var/lib/hupm/data/";
	DIR *dir = opendir(dbpath.c_str());
	int found=0;
	std::vector<const void*> blobvec;
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
			for (int i=0; i<nf; i++) {
				blobvec.push_back(sqlite3_column_blob(stmt, fields[i]));
			}
			char* name = new char[256]();
			strcpy(name, dirst->d_name);
			blobvec.push_back((void*)name);
			found=1;break;
		}
		sqlite3_finalize(stmt);sqlite3_close(db);
	}
	closedir(dir);
	if (!found) {
		throw "Package not found";
	} else {
		return blobvec;
	}
}

void extsolve(char* pkg, vector<const char*> *external) {
	char opts[] = {5};
	std::vector<const void*> fields = extractfields(pkg, opts, 1);
	std::vector<const char*> res = vecsplit((const char*)fields[0]);
	if (!res.size()) return;
	for (int i=0; i<res.size(); i++) {
		if (!vecchkelement(*external, res[i])) {
			external->push_back(res[i]);
		}
	}
}

void solve(const char* pkg, struct pkgstruct *install, struct pkgstruct *update, std::vector<int> *size) {
	char opts[3] = {4,3,6};
	std::vector<const void*> fields = extractfields(pkg, opts, 3);
	std::vector<const char*> res = vecsplit((const char*)fields[0]);
	const char* version = (const char*)fields[2];
	const char* version2;
	char* dbname = (char*)fields[3];
	//std::cout << dbname << std::endl;
	int found = 0;
	/*Check local*/
	sqlite3 *db;
	sqlite3_stmt *stmt;
	sqlite3_open("/var/lib/hupm/pkgs/insdat.db", &db);
	const char* sql = "SELECT * FROM pkgtab WHERE basename=?";
	sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, pkg, -1, NULL);
	int sqlres = sqlite3_step(stmt);
	if (sqlres == SQLITE_ROW) {
		version2 = (const char*)sqlite3_column_blob(stmt, 6);
		found = 1;
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	/*end*/
	if (!res.size()) {
		if (!vecchkelement(install->pkg, pkg)) {
			if (!found) {
				install->pkg.push_back(pkg);
				install->db.push_back(dbname);
			}
			if (found && strcmp(version, version2) && !vecchkelement(update->pkg, pkg)) {
				update->pkg.push_back(pkg);
				update->db.push_back(dbname);
			}
		}
		size->push_back(atoi((const char*)fields[1]));
	} else {
		for (int i=0; i<res.size();i++) {
			solve(res[i], install, update, size);
		}
		if (!vecchkelement(install->pkg, pkg)) {
			if (!found) {
				install->pkg.push_back(pkg);
				install->db.push_back(dbname);
			}
			if (found && strcmp(version, version2) && !vecchkelement(update->pkg, pkg)) {
				update->pkg.push_back(pkg);
				update->db.push_back(dbname);
			}
		}
		size->push_back(atoi((const char*)fields[1]));
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
		std::vector<int> pkgsize;  // Size of packages
		struct pkgstruct install;  // vector of packages to install
		struct pkgstruct update;  // vector of packages to update
		std::vector<const char*> external;  // vector of external packages to install
		for (int i=0; i<pkgreq.size(); i++) {
			std::cout << "\033[1;32mDEBUG\033[0m: ciclo: " << i << std::endl;
			DIR *dir = opendir(dbpath.c_str());
			int found=0;
			const char* version;
			const char* version2;
			while ((dirst = readdir(dir)) != NULL) {
				if (!strcmp(dirst->d_name, ".") && !strcmp(dirst->d_name, "..")) continue;
				sqlite3 *db;
				sqlite3_stmt *stmt;
				sqlite3_open((dbpath+dirst->d_name).c_str(), &db);
				int res;
				const char* sql = "SELECT * FROM pkgtab WHERE basename=?";
				sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
				sqlite3_bind_text(stmt, 1, pkgreq[i], -1, NULL);
				res = sqlite3_step(stmt);
				if (res == SQLITE_DONE) {sqlite3_finalize(stmt);sqlite3_close(db);continue;}
				else if (res == SQLITE_ROW) {
					version = (const char*)sqlite3_column_blob(stmt, 6);
					found=1;break;
				}
				sqlite3_finalize(stmt);sqlite3_close(db);
			}
			closedir(dir);
			std::cout << "\033[1;32mDEBUG\033[0m: found: " << found << std::endl;
			if (!found) {
				std::cout << "\033[1;31mError\033[0m: Package not found: " << pkgreq[i] << std::endl;
				return 1;
			} else std::cout << "\033[1;32mDEBUG\033[0m: Found pkg\n";
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
				puts("\033[1;32mDEBUG\033[0m: Package is already installed.");
				if (!strcmp(version, version2)) {
					/*Same version. Ask to reinstall*/
					/*Don't ask, notify about the reinstallation*/
					std::cout << pkgreq[i] << " is already installed and up to date. \033[1;33mIt will be reinstalled.\033[0m\n";
				} else {
					/*Different version. Update*/
					update.pkg.push_back(pkgreq[i]); /*No deps solving here*/
				}
			} else {
				puts("\033[1;32mDEBUG\033[0m: Package is not installed.");
				/*First, external*/
				extsolve(pkgreq[i], &external);
				/*Then, internal*/
				solve(pkgreq[i], &install, &update, &pkgsize);
			}
		}
		/*First, external*/
		std::cout << "Checking external deps...\n"; /*TODO: Will move it*/
		for (int i=0; i<external.size(); i++) {
			char* cmd = new char[34]();
			strcat(cmd, "pacman -Q ");
			strcat(cmd, external[i]);
			strcat(cmd, " &> /dev/null");
			int cmdres = system(cmd);
			delete[] cmd;
			if (!cmdres) {
				std::cout << "\033[1;32mDEBUG\033[0m: Pkg " << external[i] << " already installed\n";
				external.erase(external.begin() + i);
			}
		}
		if (external.size()) {
			std::cout << "The following external dependencies will be installed:\n  ";
			for (int i=0; i<external.size(); i++) {
				std::cout << external[i] << " ";
			}
			std::cout << "\n\n";
		}
		if (update.pkg.size()) {
			std::cout << "The following " << update.pkg.size() << " packages will be updated:\n  ";
			for (int i=0; i<update.pkg.size(); i++) {
				std::cout << update.pkg[i] << " ";
			}
			std::cout << "\n\n";
		}
		/*Then, internal*/
		std::cout << "The following " << install.pkg.size() << " packages will be installed:\n  ";
		for (int i=0; i<install.pkg.size(); i++) {
			std::cout << install.pkg[i] << " ";
		}
		std::cout << "\n\n";
		/*Calculate size of pkgs to install by summing
		* the size field on the database.*/
		std::cout << "Packages's total size " << sizesum(pkgsize) << "Kb\n";
		/*Ask to proceed*/
		std::cout << ":: Do you want to continue? [Y/n] ";
		char procopt = getchar();
		if (procopt != 'y' && procopt != 'Y' && procopt != '\n') {
			std::cout << "Aborted.\n";
			return 0;
		}
		/*<-- Begin installation -->*/
		/*Download every packages*/
		for (int i=0; i<install.pkg.size(); i++) {
			std::string baseurl = "https://raw.githubusercontent.com/VENOM-InstantDeath/";
			std::string basepath = "/var/lib/hupm/data/"; basepath += install.db[i];
			std::string cachepath = "/var/lib/hupm/pkg/";
			char* nm = new char[strlen(install.db[i])+1]();
			strcpy(nm, install.db[i]); nm[strlen(nm)-3] = 0;
			baseurl += nm; baseurl += "/main/";
			delete[] nm;
			sqlite3 *db;
			std::cout << "\033[1;32mDEBUG\033[0m: " << basepath << '\n';
			sqlite3_open(basepath.c_str(), &db);
			sqlite3_stmt *stmt;
			const char* sql = "SELECT * FROM pkgtab WHERE basename=?;";
			int res = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
			if (res != SQLITE_OK) {
				std::cout << "\033[1;32mDEBUG\033[0m: prepare: " << sqlite3_errmsg(db) << std::endl;
			}
			sqlite3_bind_text(stmt, 1, install.pkg[i], -1, NULL);
			res = sqlite3_step(stmt);
			const char* pkgname = (const char*)sqlite3_column_blob(stmt, 2);
			baseurl += pkgname;
			cachepath += pkgname; cachepath += ".tar.gz";
			std::cout << "\033[1;32mDEBUG\033[0m: " << baseurl << '\n';
			sqlite3_finalize(stmt);
			sqlite3_close(db);
			std::cout << "\033[1;32mDEBUG\033[0m: cachepath: " << cachepath << '\n';
			CURL *curl= curl_easy_init();
			CURLcode reqcode;
			FILE *tarball = fopen(cachepath.c_str(), "w+");
			std::cout << "\033[1;32mDEBUG\033[0m: File has been opened\n";
			curl_easy_setopt(curl, CURLOPT_URL, baseurl.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, tarball);
			std::cout << "\033[1;32mDEBUG\033[0m: Options setted\n";
			reqcode = curl_easy_perform(curl);
			if (reqcode == CURLE_OK) {
				std::cout << "\033[1;32mOK\033[0m\n";
			} else {
				std::cout << "\033[1;31mError\033[0m\n";
			}
			curl_easy_cleanup(curl);
		}
		/*Uncompress tarballs*/
		/*execute huscript*/
	}
	if (operation == "update");
	if (operation == "remove");
	/*CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION);*/
	return 0;
}
