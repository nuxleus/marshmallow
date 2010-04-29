// Marshmallow, an opensource Campfire server, by Larry Gadea trivex@gmail.com
//
// To build and run, use: make && sudo ./marshmallow
//
// To connect, set "127.0.0.1 marshmallow.campfirenow.com" in your /etc/hosts file
// and connect to marshmallow.campfirenow.com in Propane.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event.h>
#include <evhttp.h>

#include "datastore.h"

char *template_room_list = NULL;
struct evhttp *server = NULL;

// - Need http://127.0.0.1 as a constant
// - Need to actually parse templates as templates and not just read em in
// - Need to clean up handlers to use a common generator
// - Need to implement "proper" auth

void http_root_handler(struct evhttp_request *req, void *arg) {
  struct evbuffer *evbuf = evbuffer_new();
  
  // Use the "template" to output to the user. Obviously we'll need an
  // actual template parser in the future since this is uber stat.
  evbuffer_add(evbuf, template_room_list, strlen((const char*)template_room_list));
  
  evhttp_add_header(req->output_headers, "Location", "/login");
  evhttp_send_reply(req, HTTP_OK, "Found", evbuf);
  evbuffer_free(evbuf);
}

void http_statics_handler(struct evhttp_request *req, void *arg) {
  struct evbuffer *evbuf = evbuffer_new();
  
  // The static content is available in the arg param as was created
  // with add_static earlier.
  evbuffer_add(evbuf, (char *)arg, strlen((const char*)arg));
  
  // Set the Content-Type
  if (strstr(req->uri, ".css") != NULL) 
    evhttp_add_header(req->output_headers, "Content-Type", "text/css");
  if (strstr(req->uri, ".gif") != NULL) 
    evhttp_add_header(req->output_headers, "Content-Type", "image/gif");
  if (strstr(req->uri, ".png") != NULL) 
    evhttp_add_header(req->output_headers, "Content-Type", "image/png");
  if (strstr(req->uri, ".js") != NULL) 
    evhttp_add_header(req->output_headers, "Content-Type", "text/javascript");
  
  evhttp_send_reply(req, HTTP_OK, "OK", evbuf);
  evbuffer_free(evbuf);
}

char *read_file(char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Unable to open file: %s\n", filename);
    exit(1);
  }

  // Get the file size to allocate the right memory size for it
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *file_data = malloc(size);
  
  if (size == 0)
    (*file_data) = 0x00;
  
  // Read exactly the file size of data  
  if (size > 0 && !fread(file_data, size, 1, file)) {
    fprintf(stderr, "Error reading file: %s\n", filename);
    exit(1);
  }

  fclose(file);
  return file_data;
}

void add_static(struct evhttp *server, char *uri, char *local_path) {
  char *data = read_file(local_path);
  evhttp_set_cb(server, uri, http_statics_handler, (void*)data);
}

void sigint(int i) {
  fprintf(stderr, "Shutting down!\n");
  
  mds_shutdown();
  
  if (server)
    evhttp_free(server);
  
  exit(0);
}

int main(void) {
  fprintf(stderr, "Marshmallow 0.01 by Larry Gadea (trivex@gmail.com)\n\n");

  // Trap CTRL+C for shutdown
  if (signal(SIGINT, SIG_IGN) != SIG_IGN)
    signal(SIGINT, sigint);

  // Start the server
  event_init();
  server = evhttp_start("0.0.0.0", 80);
  if (!server) {
    fprintf(stderr, "Failed to start server! This is probably because you didn't run as sudo and as such port 80 isn't available.\n");
    exit(1);
  }

  // Initialize the datastore
  mds_init();
  
  // Set up handlers for the different urls
  evhttp_set_cb(server, "/", http_root_handler, NULL);
  add_static(server, "/room/295440/", "templates/room.tpl");
  add_static(server, "/room/295440/speak", "statics/blank");
  evhttp_set_gencb(server, http_statics_handler, (void *)read_file("statics/blank"));

  // Read in templates
  fprintf(stderr, "Reading templates...\n");
  template_room_list = read_file("templates/room_list.tpl");

  fprintf(stderr, "Preparing statics...\n");
  add_static(server, "/sprockets.js", "statics/sprockets.js");
  add_static(server, "/images/bottom-bg.gif", "statics/images/bottom-bg.gif");
  add_static(server, "/images/dots-white.gif", "statics/images/dots-white.gif");
  add_static(server, "/images/progress_bar.gif", "statics/images/progress_bar.gif");
  add_static(server, "/images/right-bg.gif", "statics/images/right-bg.gif");
  add_static(server, "/images/sound-on.gif", "statics/images/sound-on.gif");
  add_static(server, "/images/speak-bg.png", "statics/images/speak-bg.png");
  add_static(server, "/stylesheets/blue.css", "statics/stylesheets/blue.css");
  add_static(server, "/stylesheets/chat.css", "statics/stylesheets/chat.css");
  add_static(server, "/stylesheets/open_bar.css", "statics/stylesheets/open_bar.css");
  add_static(server, "/stylesheets/print.css", "statics/stylesheets/print.css");
  add_static(server, "/stylesheets/screen.css", "statics/stylesheets/screen.css");
  
  // Open the flood gates
  fprintf(stderr, "\nListening for connections on port 80...\n");
  event_dispatch();

  // Should never really get here, but if so, shutdown
  sigint(0);
}

