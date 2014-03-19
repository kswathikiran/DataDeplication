/*
 * ramdisk.cpp
 *
 *  Created on: Feb 8 2014
 *      Author: swathi
 */
#define FUSE_USE_VERSION 26
#include<fuse.h>
#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<map>
#include<fcntl.h>
#include<time.h>
using namespace std;

struct fs {
	bool dflag;
	string data;
	string file_name;
	off_t file_size;
	time_t last_access;
	time_t last_modified;
	time_t last_status_change;
	std::map<string, struct fs> hashobj;
};

struct fs global;
long fsize;
long lsize;

static int ramdisk_getAttribute(const char *path, struct stat *stbuff) {
	std::map<string, struct fs>::iterator it;
	struct fs *local;
	int result = 0;
	memset(stbuff, 0, sizeof(struct stat));
	char *res;
	char path1[200];
	strcpy(path1, path);
	if (strcmp(path, "/") == 0) {
		stbuff->st_mode = S_IFDIR | 0755;
		stbuff->st_nlink = 2;
		stbuff->st_size = sizeof(struct fs);
	} else {
		local = &global;
		res = strtok(path1, "/");
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				result = -ENOENT;
				return result;
			} else {
				local = &(it->second);
			}
			res = strtok(NULL, "/");
		}
		if (local->dflag == true) {
			stbuff->st_mode = S_IFDIR | 0755;
			stbuff->st_nlink = 2;
			stbuff->st_size = sizeof(struct fs);
			/*stbuff->st_atim=local->last_access;
			 stbuff->st_ctim=local->last_status_change;
			 stbuff->st_mtim=local->last_modified;*/
		} else {
			stbuff->st_mode = S_IFREG | 0777;
			stbuff->st_nlink = 1;
			stbuff->st_size = local->data.size();
			/*stbuff->st_ctim=local->last_status_change;
			 stbuff->st_atim=local->last_access;
			 stbuff->st_mtim=local->last_modified;*/

		}

	}
	return 0;

}

static int ramdisk_mkdir(const char *path, mode_t mode) {
	std::map<string, struct fs>::iterator it;
	char result = 0;
	struct fs *local;
	if(lsize > fsize)
		return -ENOMEM;
	char *res;
	char path1[200];
	strcpy(path1, path);
	local = &global;
	if (strcmp(path1, "/") == 0) {
		//local->hashobj.insert(it,std::pair<string,struct fs>(path1,struct fs));
	} else {

		res = strtok(path1, "/");
		char fname[256];
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {
				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
		struct fs newd;
		newd.dflag = true;
		newd.file_name = fname;
		newd.file_size = sizeof(struct fs);
		lsize += newd.file_size;
		local->hashobj.insert(std::pair<string, struct fs>(fname, newd));
	}
	return 0;
}

static int ramdisk_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi) {
	std::map<string, struct fs>::iterator it;
	(void) offset;
	(void) fi;
	//if (strcmp(path, "/") != 0)
	//return -ENOENT;
	struct fs *local;
	local = &global;
	int result = 0;
	char *res;
	char path1[200];
	strcpy(path1, path);
	if (strcmp(path1, "/") == 0) {
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		for (std::map<string, struct fs>::iterator it = local->hashobj.begin();
				it != local->hashobj.end(); ++it)
			filler(buf, it->first.c_str(), NULL, 0);
	} else {
		local = &global;
		res = strtok(path1, "/");
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				result = -ENOENT;
				return result;
			} else {

				local = &(it->second);
			}
			res = strtok(NULL, "/");
		}

		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		for (std::map<string, struct fs>::iterator it = local->hashobj.begin();
				it != local->hashobj.end(); ++it) {

			filler(buf, it->first.c_str(), NULL, 0);
		}
	}
	return 0;
}

static int ramdisk_rmdir(const char *path) {
	std::map<string, struct fs>::iterator it;
	int result = 0;
	char *res;
	char path1[256];
	strcpy(path1, path);

	if (strcmp(path1, "/") == 0) {
		//cout << "cannot delete mount point" << endl;
	} else {				//res = strtok(path1, "/");
		struct fs *local;
		char fname[256];
		local = &global;
		//cout << "Before strtok" << endl;
		res = strtok(path1, "/");
		struct fs *rmd;
		rmd = local;
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				result = -ENOENT;
				return result;
			} else {
				rmd = local;
				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}

		//cout << "Local folder name :" << local->file_name << endl;
		//cout << "Parent :" << rmd->file_name << endl;
		if (local->hashobj.size() == 0) {
			//cout << "Inside directory empty" << endl;
			lsize -= local->file_size;
			rmd->hashobj.erase(rmd->hashobj.find(fname));
		} else {
			//cout << "Inside directory not empty" << endl;
			result = -ENOTEMPTY;
			return result;
		}
	}
	return 0;
}

static int ramdisk_create(const char *path, mode_t mode,
		struct fuse_file_info *fi) {
	std::map<string, struct fs>::iterator it;
	int result = 0;
	struct fs *local;
	char *res;
	char path1[200];
	strcpy(path1, path);
	local = &global;
	if (strcmp(path1, "/") == 0) {
		//local->hashobj.insert(it,std::pair<string,struct fs>(path1,struct fs));
	} else {

		res = strtok(path1, "/");
		char fname[256];
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {
				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
		struct fs newd;
		newd.dflag = false;
		newd.file_name = fname;
		newd.file_size = 0;
		local->hashobj.insert(std::pair<string, struct fs>(fname, newd));
	}
	return 0;

}

static int ramdisk_open(const char *path, struct fuse_file_info *fi) {
	std::map<string, struct fs>::iterator it;
	int result = 0;
	struct fs *local;
	char *res;
	char path1[200];
	strcpy(path1, path);
	local = &global;
	if (strcmp(path1, "/") == 0) {
		//local->hashobj.insert(it,std::pair<string,struct fs>(path1,struct fs));
	} else {

		res = strtok(path1, "/");
		char fname[256];
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {

				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
	}
	return 0;
}

static int ramdisk_read(const char *path, char *buff, size_t size, off_t offset,
		struct fuse_file_info *fi) {
	size_t len;
	(void) fi;
	std::map<string, struct fs>::iterator it;
	int result = 0;
	struct fs *local;
	char *res;
	char path1[200];
	strcpy(path1, path);
	local = &global;
	if (strcmp(path1, "/") == 0) {
		//local->hashobj.insert(it,std::pair<string,struct fs>(path1,struct fs));
	} else {

		res = strtok(path1, "/");
		char fname[256];
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {

				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
	}

	len = local->file_size;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buff, local->data.c_str() + offset, size);
	} else
		size = 0;

	return size;

}

static int ramdisk_write(const char *path, const char *buff, size_t size,
		off_t offset, struct fuse_file_info *fi) {
	(void) fi;
	std::map<string, struct fs>::iterator it;
	struct fs *local;
	char *res;
	char path1[200];
	strcpy(path1, path);
	local = &global;
	if (strcmp(path1, "/") == 0) {
		//local->hashobj.insert(it,std::pair<string,struct fs>(path1,struct fs));
	} else {
		res = strtok(path1, "/");
		char fname[256];
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {
				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
		int len = local->file_size;
		if (offset+size < len) {

			//local->file_size=offset+size;
			local->data.replace(offset, size, buff);

		} else {
			if(lsize > fsize)
				return -ENOMEM;
			local->data.resize(offset + size);
			local->data.replace(offset, size, buff);
			local->file_size=offset+size;
			lsize += (local->file_size - len);
		}

	}
	return size;
}

static int ramdisk_truncate(const char *path, off_t offset) {
	std::map<string, struct fs>::iterator it;
	int result = 0;
	char *res;
	char path1[256];
	strcpy(path1, path);

	if (strcmp(path1, "/") == 0) {
		//cout << "cannot delete mount point" << endl;
	} else {				//res = strtok(path1, "/");
		struct fs *local;
		char fname[256];
		local = &global;
		//cout << "Before strtok" << endl;
		res = strtok(path1, "/");
		struct fs *rmd;
		//rmd = local;
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				result = -ENOENT;
				return result;
			} else {
				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
		long len  = local->file_size;
		local->data.erase(local->data.begin() + offset, local->data.end());
		local->file_size = offset;
		lsize -= (len - local->file_size);
	}

	return 0;
}

static int ramdisk_unlink(const char *path) {
	int result = 0;
	std::map<string, struct fs>::iterator it;
	struct fs *local;
	char *res;
	char path1[200];
	strcpy(path1, path);
	local = &global;
	if (strcmp(path1, "/") == 0) {
		//local->hashobj.insert(it,std::pair<string,struct fs>(path1,struct fs));
	} else {
		res = strtok(path1, "/");
		char fname[256];
		struct fs *file;
		while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {
				file = local;
				local = &(it->second);
			}
			strcpy(fname, res);
			res = strtok(NULL, "/");
		}
		lsize -= local->file_size;
		file->hashobj.erase(file->hashobj.find(fname));

	}
	return result;
}


static int ramdisk_rename(const char *src, const char *des)
{int result = 0;

struct fs *local,*local1, source, *parentsource, *parentdestination, destination;
local = &global;
char *res;
char src1[256];
char des1[256];
char sname[256];
char dname[256];
strcpy(src1, src);
strcpy(des1, des);
struct fs from;
std::map<string, struct fs>::iterator it,it1;
//strcpy(src1,src);
res = strtok(src1,"/");
	while (res != NULL) {
			it = local->hashobj.find(res);
			if (it == local->hashobj.end()) {
				result = -ENOENT;
				return result;
			} else {
				parentsource = local;
				local = &(it->second);
			}
			strcpy(sname, res);
			res = strtok(NULL, "/");
		}
			it = parentsource->hashobj.find(sname);
if(it == parentsource->hashobj.end())
{
	return -ENOENT;
}
else
{
source = it->second;
parentsource->hashobj.erase(parentsource->hashobj.find(sname));
}
res = strtok(des1,"/");
local1 = &global;
while (res != NULL) {
			it1 = local1->hashobj.find(res);
			if (it1 == local1->hashobj.end()) {
				//result = -ENOENT;
				//return result;
			} else {
				parentdestination = local1;
				local1 = &(it1->second);
			}
			strcpy(dname, res);
			res = strtok(NULL, "/");
		}
if(!source.dflag)
{
if(local1->file_name.compare(dname)==0)
{
parentdestination->hashobj.erase(parentdestination->hashobj.find(dname));
source.file_name = dname;
parentdestination->hashobj.insert(std::pair<string,struct fs>(source.file_name,source));
}
else
{
	source.file_name = dname;
local1->hashobj.insert(std::pair<string,struct fs>(source.file_name,source));
}
}
else
{
	//cout<<"Parent "<< local1->file_name<<endl;
	//cout<<"Folder name"<<dname<<endl;
	if(local1->file_name.compare(dname)==0)
	{
//	parentdestination->hashobj.erase(parentdestination->hashobj.find(dname));
//	source.file_name = dname;
	local1->hashobj.insert(std::pair<string,struct fs>(source.file_name,source));
	}
	else
	{
	source.file_name = dname;
	local1->hashobj.insert(std::pair<string,struct fs>(source.file_name,source));
	}


}

return 0;
}

static struct fuse_operations ramdisk_ops;

int main(int argc, char *argv[]) {
	if(argc<3)
	exit(0);
    fsize=atoi(argv[2])*1024*1024;
    lsize=0;
	global.dflag = true;
	global.file_name = "/";
	global.file_size = sizeof(struct fs);
	global.last_access = time(0);
	global.last_modified = time(0);
	global.last_status_change = time(0);

	ramdisk_ops.getattr = ramdisk_getAttribute;
	ramdisk_ops.mkdir = ramdisk_mkdir;
	ramdisk_ops.readdir = ramdisk_readdir;
	ramdisk_ops.rmdir = ramdisk_rmdir;
	ramdisk_ops.create = ramdisk_create;
	ramdisk_ops.open = ramdisk_open;
	ramdisk_ops.read = ramdisk_read;
	ramdisk_ops.write = ramdisk_write;
	ramdisk_ops.unlink = ramdisk_unlink;
	ramdisk_ops.truncate = ramdisk_truncate;
	ramdisk_ops.rename = ramdisk_rename;
	argv[2] = NULL;

	return fuse_main(argc-1, argv,&ramdisk_ops, NULL);
}
