#include <ubus-intf.h>

// globals
struct 	blob_buf 	gMsg;

// function prototypes
void ubusDataCallback(struct ubus_request *req, int type, struct blob_attr *msg);


// initialize the blob_buf
void ubusInit()
{
	onionPrint(ONION_SEVERITY_DEBUG_EXTRA, ">>> Running blob_buf_init\n");
	blob_buf_init(&gMsg, 0);
}

// free the blob_buf
void ubusFree()
{
	onionPrint(ONION_SEVERITY_DEBUG_EXTRA, ">>> Running blob_buf_free\n");
	blob_buf_free(&gMsg);
}

// call ubus function 
int ubusCall (char* group, char* method, char* params, char* respUrl)
{
	int 	status;
	int 	groupId;
	char 	*ubus_socket;
	struct 	ubus_context 		*ctx;

	// initialize ubus context
	ctx 	= ubus_connect(ubus_socket);

	if (!ctx) {
		return EXIT_FAILURE;
	}

	// lookup the ubus group
	onionPrint(ONION_SEVERITY_DEBUG_EXTRA, ">>> Running ubus_lookup_id\n");
	status 	= ubus_lookup_id(ctx, group, &groupId);

	if (status == EXIT_SUCCESS) {
		// add the params 
		if (!blobmsg_add_json_from_string(&gMsg, params) ) {
			onionPrint(ONION_SEVERITY_FATAL, "ERROR: Failed to parse ubus parameter data\n");
			return EXIT_FAILURE;
		}

		// make the ubus call
		onionPrint(ONION_SEVERITY_DEBUG, ">> Launching ubus call\n");
		status 	= ubus_invoke(	ctx, groupId, method, 					// ubus context, group id, method string
								gMsg.head, ubusDataCallback, respUrl,	// blob attr, handler function, priv
								30000);		// timeout
	}

	// clean-up
	ubus_free (ctx);

	return 	status;
}

// ubus handler
void ubusDataCallback(struct ubus_request *req, int type, struct blob_attr *msg)
{
	int 	status;
	char 	*body;
	//char 	*respUrl;
	char 	respUrl[BUFFER_LENGTH];

	// check for valid response
	if (!msg) {
		return;
	}

	// read the response data
	strcpy(respUrl, (char *)req->priv);

	// convert the blobmsg json to a string
	body = blobmsg_format_json_indent(msg, true, 0);
	//body = blobmsg_format_json(msg, false);

	// send the curl response
	status 	= curlPost(respUrl, body);
	

	
	free(body);
	//printf (">> freed str: '%s'\n", *(char *)req->priv);
}