#include <iostream>
#include <fstream>
#include <cstring>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <sqlite3.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <json/json.h>
using namespace std;
using namespace Json;

char* lowcase(const char* s) {
	char* r = new char[strlen(s)];
	for (int i=0;i<strlen(s);i++) {
		r[i] = tolower(s[i]);
	}
	return r;
}

int chtoint(char* x) {
	int n=0;
	for (int i=0;i<strlen(x);i++) {
		n*=10;
		n+=x[i]-48;
	}
	return n;
}

int dbretrieve(void *data, int size, char **value, char **column) {
	pair<int*,string*>* p = (pair<int*,string*>*)data;
	(*p).first[0] = chtoint(value[0]);
	(*p).first[1] = chtoint(value[8]);
	for (int i=0;i<7;i++) {
		(*p).second[i] = value[i+1];
	}
	return 0;
}

int chpackexist(void *data, int size, char **value, char **column) {
	int *x = (int*)data;
	*x = 1;
	if (!size) return 0;
	return 0;
}

int main(int argc, char *argv[]) {
	const char* red= "\033[1;31m";
	const char* yellow= "\033[1;33m";
	const char* nm = "\033[0m";
	const char* oldpwd = get_current_dir_name();
	if (argc == 1) {
		cout << red << "error: " << nm << "no se ha especificado una operación (usa -h para ayuda)\n";
		return 1;
	}
	int uid=getuid();
	if (!strcmp(argv[1], "install") || !strcmp(argv[1], "i")) {
		cout << "install\n";
		// Comprobar si sudo
		if (uid) {
			cout <<red<<"error: "<<nm << "esta operación requiere permisos elevados\n";
			return 1;
		}
		// Comprobar que se haya especificado un paquete
		if (argc < 3) {
			cout <<red<<"error: "<<nm<<"la operación espera recibir dos argumentos como mínimo\n";
			return 1;
		}
		// Comprobar si existe la config, sino la crea
		struct stat statbuff;
		if (stat("/var/lib/hupm/conf.json", &statbuff)) {
			fstream tempf("/var/lib/hupm/conf.json", ios::out);
			tempf << "{\n\"sysutils\": \"https://raw.githubusercontent.com/VENOM-InstantDeath/sysutils/main/\"\n}";
			cout << "created conf\n";
			tempf.close();
		}
		// Cargar la config
		Json::Reader reader;
		Json::Value root;
		fstream config("/var/lib/hupm/conf.json", ios::in);
		string jstr,line;
		while (getline(config, line)) jstr += line;
		if (!reader.parse(jstr, root)) {
			cout << "Se ha producido un error al cargar la config\n";
			return 1;
		}
		Json::Value::Members dbinconf=root.getMemberNames();
		vector<sqlite3*> databases;
		// Cargar bases de datos
		for (int i=0;i<dbinconf.size();i++) {
			stringstream ss;
			ss << "/var/lib/hupm/db/" << dbinconf[i] <<".db";
			if (stat(ss.str().c_str(),&statbuff)) {
				cout <<red<<"error: "<<nm<<"faltan bases de datos\n";
				cout << dbinconf[i] << " no existe\n";
				return 1;
			}
			sqlite3* DB;
			sqlite3_open(ss.str().c_str(),&DB);
			databases.push_back(DB);
		}
		// Comprobar si el paquete existe
		int pckexists=0;
		stringstream sql;
		sql << "SELECT * FROM packs WHERE name=\""<<argv[2]<<"\";";
		for (int i=0;i<databases.size();i++) {
			int check = 0;
			sqlite3_exec(databases[i], sql.str().c_str(),chpackexist,&check,NULL);
			if (check) {
				pckexists = i+1;
				cout << "package exists in " << dbinconf[i] << endl;
			}
		}
		// Si el paquete existe:
		if (pckexists) {
			pair<int*,string*> dbdata;
			dbdata.first = new int[2];
			dbdata.second = new string[7];
			sqlite3_exec(databases[pckexists-1],sql.str().c_str(),dbretrieve,&dbdata,NULL);
			string choice;
			if (dbdata.first[1]) {
				cout << "El paquete ya está instalado, ¿Deseas reinstalarlo? [Y/n] ";
				getline(cin,choice);
			}
			if (choice.size() && !strcmp(lowcase(choice.c_str()),"y")) return 0;
			// Obtener el nombre del paquete
			string pkgname = dbdata.second[2];
			stringstream pkgpath;
			pkgpath << "/var/lib/hupm/packs/"<<pkgname.c_str()<<".tar.gz";
			// Ruta del nuevo paquete
			FILE* downpack = fopen(pkgpath.str().c_str(), "w");
			stringstream pkgurl;
			pkgurl << root[dbinconf[pckexists-1]].asString() << pkgname.c_str() << ".tar.gz";
			// Descargar el paquete
			CURL* curl = curl_easy_init();
			CURLcode res;
			curl_easy_setopt(curl, CURLOPT_URL,pkgurl.str().c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, downpack);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:91.0) Gecko/20100101 Firefox/91.0");
			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			fflush(downpack);fclose(downpack);
			chdir("/var/lib/hupm/packs");
			//Instalation here
			string cmd("tar -xf ");cmd.append(pkgpath.str().c_str());
			system(cmd.c_str());
			chdir(argv[2]);
			system("bash .huscript");
			chdir("..");
			remove(pkgpath.str().c_str());
			//End of instalation
			chdir(oldpwd);
			//Mark as installed
			stringstream installed;
			installed<<"UPDATE packs SET installed=1 WHERE name=\""<<argv[2]<<"\";";
			sqlite3_exec(databases[pckexists-1],installed.str().c_str(),NULL,NULL,NULL);
			//End of marking as installed
			cout << pkgname << " ha sido instalado correctamente!\n";
			return 0;
		}
		cout <<red<<"error: "<<nm<<"el paquete solicitado no está disponible\n";
		return 1;
	}
	else if (!strcmp(argv[1], "update") || !strcmp(argv[1], "u")) {
		cout << "update\n";
		if (uid) {
			cout <<red<<"error: "<<nm << "esta operación requiere permisos elevados\n";
			return 1;
		}
		if (argc > 2) {
			cout <<yellow<<"advertencia: "<<nm<<"la operación no espera recibir ningún argumento\n";
		}
		struct stat statbuff;
		if (stat("/var/lib/hupm/conf.json", &statbuff)) {
			fstream tempf("/var/lib/hupm/conf.json", ios::out);
			tempf << "{\n\"sysutils\": \"https://raw.githubusercontent.com/VENOM-InstantDeath/sysutils/main/\"\n}";
			cout << "created conf\n";
			tempf.close();
		}
		Json::Reader reader;
		Json::Value root;
		fstream config("/var/lib/hupm/conf.json", ios::in);
		string jstr,line;
		while (getline(config, line)) jstr += line;
		if (!reader.parse(jstr, root)) {
			cout << "Se ha producido un error al cargar la config\n";
			return 1;
		}
		Json::Value::Members dbinconf=root.getMemberNames();
		for (int i=0;i<dbinconf.size();i++) {
			cout << "Obteniendo "<<dbinconf[i]<<"...";
			stringstream dburl;
			dburl << "https://raw.githubusercontent.com/VENOM-InstantDeath/"<<dbinconf[i]<<"/main/"<<dbinconf[i]<<".db";
			stringstream dbpath;
			dbpath<<"/var/lib/hupm/db/"<<dbinconf[i]<<".db";
			FILE* db = fopen(dbpath.str().c_str(), "w");
			CURL* curl = curl_easy_init();
			CURLcode res;
			curl_easy_setopt(curl, CURLOPT_URL,dburl.str().c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, db);
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64; rv:91.0) Gecko/20100101 Firefox/91.0");
			curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			fflush(db);fclose(db);
			cout << "OK!\n";
		}
		return 0;
	}
	else if (!strcmp(argv[1], "remove") || !strcmp(argv[1], "r")) {
		cout << "remove\n";
	}
	else if (!strcmp(argv[1], "search") || !strcmp(argv[1], "s")) {
		cout << "search\n";
	}
	else {
		cout <<red<<"error: "<<nm<<"la operación especificada no existe\n";
	}
	return 0;
}
