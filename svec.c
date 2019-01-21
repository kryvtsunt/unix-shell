// svec.c
// Author: Nat Tuck
// 3650F2017, Challenge01 Hints

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "svec.h"

	svec*
make_svec()
{
	svec* sv = malloc(sizeof(svec));
	sv->size = 0;
	sv->cap  = 4;
	sv->data = malloc(4 * sizeof(char*));
	memset(sv->data, 0, 4 * sizeof(char*));
	return sv;
}

	void
free_svec(svec* sv)
{
	int ii;
	for (ii = 0; ii < sv->size; ++ii) {
		if (sv->data[ii] != 0) {
			free(sv->data[ii]);
		}
	}
	free(sv->data);
	free(sv);
}

// split svec by the given string (into array of svecs )
svec** split_svec(svec* sv, char* str)
{
	svec** ssv = malloc(10*sizeof(svec*));
	memset(ssv,0,10*sizeof(svec*));
	int ii = 0;
	int cc = 0;
	ssv[cc] = make_svec();
	for(ii=0; ii <sv->size; ++ii) {
		if (strcmp(sv->data[ii], str) ==0){ cc++; ssv[cc] = make_svec(); }
		else { 
			svec_push_back(ssv[cc],sv->data[ii]); 
		}
	}
	return ssv;
}

// split svec in two halfs when the first case of a given string occures
svec** split2_svec(svec* sv, char* str)
{
	svec** ssv = malloc(2*sizeof(svec*));
	memset(ssv,0,2*sizeof(svec*));
	int ii = 0;
	int cc = 0;
	ssv[cc] = make_svec();
	for(ii=0; ii < sv->size; ++ii) {
		if ((cc<1)&&(strcmp(sv->data[ii], str) ==0)){ cc++; ssv[cc] = make_svec(); }
		else { 
			svec_push_back(ssv[cc],sv->data[ii]); 
		}
	}
	return ssv;
}
// check if svec contains given string
int svec_contains(svec* sv, char* str)
{
	int ii = 0;
	for(ii=0; ii <sv->size; ++ii) {
		if (strcmp(sv->data[ii], str) ==0) return 1;
	}
	return 0;
}

	char*
svec_get(svec* sv, int ii)
{
	assert(ii >= 0 && ii < sv->size);
	return sv->data[ii];
}

	void
svec_put(svec* sv, int ii, char* item)
{
	assert(ii >= 0 && ii < sv->size);
	sv->data[ii] = strdup(item);

}

void svec_push_back(svec* sv, char* item)
{
	int ii = sv->size;

	if (ii >= sv->cap) {
		sv->cap *= 2;
		sv->data = (char**) realloc(sv->data, sv->cap * sizeof(char*));
	}

	sv->size = ii + 1;
	svec_put(sv, ii, item);
}

