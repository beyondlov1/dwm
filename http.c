
/*#include <stdio.h>*/
/*#include <string.h>*/
/*#include <stdlib.h>*/
// sudo apt-get install libcurl4-openssl-dev
#include <curl/curl.h>

struct HttpResponse {
	CURLcode code;
	size_t size;
	char *content;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *result)
{
	size_t realsize = size * nmemb;
	struct HttpResponse *mem = (struct HttpResponse *) result;

	char *ptr = realloc(mem->content, mem->size + realsize + 1);
	if(!ptr) {
		return 0;
	}
	mem->content = ptr;
	memcpy(&(mem->content[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->content[mem->size] = 0;
	return realsize; 
}

int 
httppost(char *url, char *params, struct HttpResponse *resp)
{
	CURL *curl;
	CURLcode res;


	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);

		/*post*/
		/*char *params = "clients=winaction,aaa";*/
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params); // params  

		// http3
		/*curl_easy_setopt(curl, CURLOPT_HTTP_VERSION,(long)CURL_HTTP_VERSION_3);*/

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp); 
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK)
			return 0;
		curl_easy_cleanup(curl);
		return 1;
	}
	return 0;
}
int 
httpget(char *url, struct HttpResponse *resp)
{
	CURL *curl;
	CURLcode res;


	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);

		//post
		/*char *params = "clients=winaction,aaa";*/
		/*curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params); // params  */

		// http3
		/*curl_easy_setopt(curl, CURLOPT_HTTP_VERSION,(long)CURL_HTTP_VERSION_3);*/

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);  
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)resp); 
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK)
			return 0;
		curl_easy_cleanup(curl);
	}
	return 1;
}
