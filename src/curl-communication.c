#include <curl-communication.h>


/* Auxiliary function that waits on the socket. */ 
static int wait_on_socket(curl_socket_t sockfd, int for_recv, long timeout_ms)
{
	struct timeval tv;
	fd_set infd, outfd, errfd;
	int res;

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec= (timeout_ms % 1000) * 1000;

	FD_ZERO(&infd);
	FD_ZERO(&outfd);
	FD_ZERO(&errfd);

	FD_SET(sockfd, &errfd); /* always check for error */ 

	if(for_recv)
	{
		FD_SET(sockfd, &infd);
	}
	else
	{
		FD_SET(sockfd, &outfd);
	}

	/* select() returns the number of signalled sockets or -1 */ 
	res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
	return res;
}

// global curl init
int curlInit() 
{
	curl_global_init(CURL_GLOBAL_SSL);
}

// global curl cleanup
int curlCleanup() 
{
	curl_global_cleanup();
}

size_t onListenResponse (void *buffer, size_t size, size_t nmemb, void *userp){
	printf("ZH: \r\n");
	printf(buffer);
}

int parseRecvData(int nread, char* buf)
{
	int 	i;
	int 	status;
	char 	*ref;
	json_object 	*jobj;

	// parse the json from the received data into a char
	ref = &buf[0];
	for(i =0; i<nread; i++){
		if(ref[0]!='{'){
			ref++;
		}
		else {
			break;
		}
	}

	onionPrint(ONION_SEVERITY_DEBUG_EXTRA, ">> Data:\n%s\n", ref);

	// parse the char into a json object
	if (strlen(ref) > 0) {
		jobj = json_tokener_parse(ref);

		// device client - process the command received from the server
		status 	= dcProcessRecvCommand(jobj);
	}	
}

// listen to device server
int curlListen (char* host, char* request)
{
	CURL 		*req;
	CURLcode 	res;
	int 		status	= EXIT_SUCCESS;


	/* Minimalistic http request */ 
	curl_socket_t sockfd; /* socket */ 
	long sockextr;
	size_t iolen;
	curl_off_t nread;
	
	// initialize request
	req 	= curl_easy_init();
	if(!req){
		return EXIT_FAILURE;
	}

	// set the URL
	curl_easy_setopt(req, CURLOPT_URL, host);
	// 	curl_easy_setopt(req, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(req, CURLOPT_CONNECT_ONLY, 1L);

	// perform the action
	onionPrint(ONION_SEVERITY_DEBUG, ">> Connecting to host '%s' to listen\n", host);
	res = curl_easy_perform(req);
	if(CURLE_OK != res)
	{
		onionPrint(ONION_SEVERITY_FATAL, "Error: %s\n", strerror(res));
		return EXIT_FAILURE;
	}
	res = curl_easy_getinfo(req, CURLINFO_LASTSOCKET, &sockextr);
 
	if(CURLE_OK != res)
	{
	  onionPrint(ONION_SEVERITY_FATAL, "Error: %s\n", curl_easy_strerror(res));
	  return EXIT_FAILURE;
	}

	sockfd = sockextr;
 
	/* wait for the socket to become ready for sending */ 
	if(!wait_on_socket(sockfd, 0, 60000L))
	{
	  onionPrint(ONION_SEVERITY_FATAL, "Error: timeout.\n");
	  return EXIT_FAILURE;
	}
	
	onionPrint(ONION_SEVERITY_INFO, "> Sending request.\n");
	onionPrint(ONION_SEVERITY_DEBUG_EXTRA, "> Request: '%s'\n", request);
	/* Send the request. Real applications should check the iolen
	 * to see if all the request has been sent */ 
	res = curl_easy_send(req, request, strlen(request), &iolen);
 
	if(CURLE_OK != res)
	{
		onionPrint(ONION_SEVERITY_FATAL, "Error: %s\n", curl_easy_strerror(res));
		return EXIT_FAILURE;
	}
	
	/* read the response */ 
	onionPrint(ONION_SEVERITY_INFO, "> Reading response.\n");
	for(;;)
	{
		char buf[BUFFER_LENGTH];
		memset(&buf[0], 0, sizeof(buf));	// clear the buffer

		onionPrint(ONION_SEVERITY_DEBUG, ">> Waiting on socket...\n");
		wait_on_socket(sockfd, 1, 60000L);
		res = curl_easy_recv(req, buf, BUFFER_LENGTH, &iolen);

		if(CURLE_OK != res) {
			break;
		}

		nread = (curl_off_t)iolen;

		onionPrint(ONION_SEVERITY_INFO, "\n> Received %" CURL_FORMAT_CURL_OFF_T " bytes.\n", nread);
		onionPrint(ONION_SEVERITY_DEBUG_EXTRA+1, ">> Received data: '%s'\n", buf);

		// parse the json from the received data;
		status 	= parseRecvData(nread, buf);
	}
	
	curl_easy_cleanup(req);
	return status;
}

// perform an http post operation
int curlPost(char* url, char* postData)
{
	int status;
	CURL *curl;
	CURLcode res;

	onionPrint(ONION_SEVERITY_DEBUG, ">> Sending POST to url: '%s', post data: '%s'\n", url, postData);

	// get the handle
	curl = curl_easy_init();

	// check that the handle is ok
	if(curl) {
		// set the URL that will receive the POST
		curl_easy_setopt(curl, CURLOPT_URL, url);
		// specify the POST data
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);

		// Perform the request, res will get the return code
		res = curl_easy_perform(curl);
		
		// Check for errors
		if(res != CURLE_OK) {
			onionPrint(ONION_SEVERITY_FATAL, "Error: %s\n", curl_easy_strerror(res));
			status 	=  EXIT_FAILURE;
		}

		
		// cleanup
		curl_easy_cleanup(curl);
	}

	return status;
}





