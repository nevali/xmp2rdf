/*
 * Copyright 2012 Mo McRoberts.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <librdf.h>

static const char *short_program_name = "xmp2rdf";

static const char *input_filename;
static FILE *output_file = NULL;
static const char *base_uri = NULL;
static const char *output_format = "turtle";

static librdf_world *world;
static librdf_model *model;
static librdf_parser *parser;
static librdf_storage *storage;
static librdf_uri *base;
static librdf_serializer *serial;

static void usage(void);
static int process_document(xmlDoc *doc);
static int process_rdf(xmlDoc *doc, xmlNode *rdf);
static int process_container(xmlDoc *doc, xmlNode *root);

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [OPTIONS] INPUT.XMP [OUTPUT]\n", short_program_name);
	fprintf(stderr, "If OUTPUT is not specified, output is written to standard output.\n\n");
	fprintf(stderr, "OPTIONS are one or more of:\n");
	fprintf(stderr, "  -h        Display this usage message\n");
	fprintf(stderr, "  -f FORMAT Specify output format (turtle, ntriples, rdfxml, rdfxmla, json,\n"
			"            html, rss). Default is turtle.\n");
	fprintf(stderr, "  -b URI    Specify base URI for parsing and serialisation\n");
}

static int
rdf_init(void)
{
	world = librdf_new_world();
	if(!world)
	{
		fprintf(stderr, "%s: failed to initialise RDF library\n", short_program_name);
		return -1;
	}
	storage=librdf_new_storage(world, "memory", NULL, NULL);
	if(!storage)
	{
		fprintf(stderr, "%s: failed to create RDF graph storage\n", short_program_name);
		return -1;
	}
	model = librdf_new_model(world, storage, NULL);
	if(!model)
	{
		fprintf(stderr, "%s: failed to create RDF graph\n", short_program_name);
		return -1;
	}
	parser = librdf_new_parser(world, NULL, NULL, NULL);
	if(!parser)
	{
		fprintf(stderr, "%s: failed to create RDF/XML parser\n", short_program_name);
		return -1;
	}
	serial = librdf_new_serializer(world, output_format, NULL, NULL);
	if(!serial)
	{
		fprintf(stderr, "%s: failed to create serializer for '%s'\n", short_program_name, output_format);
		return -1;
	}
	if(base_uri)
	{
		base = librdf_new_uri(world, (const unsigned char *) base_uri);
	}
	else
	{
		base = librdf_new_uri_from_filename(world, input_filename);
	}
	if(!base)
	{
		fprintf(stderr, "%s: failed to create base URI\n", short_program_name);
		return -1;
	}
	return 0;
}

static int
process_rdf(xmlDoc *doc, xmlNode *rdf)
{
	xmlChar *p;
	int size;
	xmlNode *node, *nnode;

	xmlReconciliateNs(doc, rdf);
	for(node = doc->children; node; node = nnode)
	{
		nnode = node->next;
		if(node->type != XML_ELEMENT_NODE)
		{
			xmlUnlinkNode(node);
		}
	}
	xmlDocSetRootElement(doc, rdf);
	p = NULL;
	size = 0;
	xmlDocDumpMemory(doc, &p, &size);
	if(!p)
	{
		fprintf(stderr, "%s: failed to serialise XML to buffer\n", input_filename);
		return -1;
	}
	if(librdf_parser_parse_string_into_model(parser, (const unsigned char *) p, base, model))
	{
		fprintf(stderr, "%s: failed to parse RDF/XML as RDF\n", input_filename);
		return -1;
	}
	if(librdf_serializer_serialize_model_to_file_handle(serial, output_file ? output_file : stdout, base, model))
	{
		fprintf(stderr, "%s: failed to serialise RDF to output file\n", input_filename);
		return -1;
	}
	xmlFree(p);
	return 0;
}

static int
process_container(xmlDoc *doc, xmlNode *root)
{
	xmlNode *node;

	for(node = root->children; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE)
		{
			continue;
		}
		if(node->ns && !strcmp((const char *) node->ns->href, "http://www.w3.org/1999/02/22-rdf-syntax-ns#") && !strcmp((const char *) node->name, "RDF"))
		{
			return process_rdf(doc, node);
		}		
	}
	return -1; 
}

static int
process_document(xmlDoc *doc)
{
	xmlNode *root, *node;
	int processed;

	processed = 0;
	root = xmlDocGetRootElement(doc);
	for(node = root; node; node = node->next)
	{
		if(node->type != XML_ELEMENT_NODE)
		{
			continue;
		}
		if(node->ns && !strcmp((const char *) node->ns->href, "adobe:ns:meta/"))
		{
			if(!strcmp((const char *) node->name, "xmpmeta") || !strcmp((const char *) node->name, "xapmeta"))
			{
				if(processed)
				{
					fprintf(stderr, "%s: Warning: multiple XMP metadata containers found; only the first will be processed\n", input_filename);
				}
				else
				{
					if(process_container(doc, node))
					{
						return -1;
					}
					processed++;
				}
				return 0;
			}
		}
		else if(node->ns && !strcmp((const char *) node->ns->href, "http://www.w3.org/1999/02/22-rdf-syntax-ns#") && !strcmp((const char *) node->name, "RDF"))
		{
			if(processed)
			{
				fprintf(stderr, "%s: Warning: multiple XMP metadata containers found; only the first will be processed\n", input_filename);
			}
			else
			{
				if(process_rdf(doc, node))
				{
					return -1;
				}
				processed++;
			}
		}
		if(node->ns && node->ns->href && node->ns->href[0])
		{
			fprintf(stderr, "%s: Warning: unexpected element %s within the %s namespace\n", input_filename, node->name, node->ns->href);
		}
		else
		{
			fprintf(stderr, "%s: Warning: unexpected element %s within the default namespace\n", input_filename, node->name);
		}
	}
	fprintf(stderr, "%s: no XMP metadata container found\n", input_filename);
	return -1;
}

int
main(int argc, char **argv)
{
	const char *t;
	xmlDoc *doc;
	int c;

	if(NULL != (t = strrchr(argv[0], '/')))
	{
		t++;
		short_program_name = t;
	}
	else
	{
		short_program_name = argv[0];
	}
	while(-1 != (c = getopt(argc, argv, "hf:b:")))
	{
		switch(c)
		{
		case 'f':
			output_format = optarg;
			break;
		case 'b':
			base_uri = optarg;
			break;
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}
	if(argc - optind < 1 || argc - optind > 2)
	{
		usage();
		exit(EXIT_FAILURE);
	}
	input_filename = argv[optind];	
	if(rdf_init())
	{
		exit(EXIT_FAILURE);
	}
	doc = xmlParseFile(input_filename);
	if(!doc)
	{
		fprintf(stderr, "%s: failed to parse XML document\n", input_filename);
		exit(EXIT_FAILURE);
	}
	if(argc - optind > 1)
	{
		output_file = fopen(argv[optind + 1], "w");
		if(!output_file)
		{
			perror(argv[optind + 1]);
			exit(EXIT_FAILURE);
		}
	}
	if(process_document(doc))
	{
		exit(EXIT_FAILURE);
	}
	return 0;
}
