/*                                                                  -*- c++ -*-
 * A simple Meat compiler for building Libraries and Archives.
 *
 * Copyright (c) 2017 Ron R Wills <ron.rwsoft@gmail.com>
 *
 * This file is part of Meat.
 *
 * Meat is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Meat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Meat.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <meat.h>
#include <meat/datastore.h>
#include <meat/utilities.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "getopt.h"

#ifdef TESTING
#include <testing.h>
#endif

std::deque<const char *> source_files;
std::string out_file;
std::string app_class;

meat::Reference library;

/** Display simple help for the -h option
 */
static void help() {
  std::cout << "Meat Compiler v" << VERSION << "\n\n";
  std::cout << "grinder [-l libname|-a archname] [-i path] source ...\n";
  std::cout << "grinder -h\n";
  std::cout << "  -i path      Include path to the library search\n";
  std::cout << "  -c appclass  Compile a library\n";
  std::cout << "  -s           Run the source as an intrepreted script\n";
  std::cout << "  -# text      Return a hash value for the text\n";
  std::cout << "  -h           Displays this help" << std::endl;
}

static std::string library_name(const std::string &filename) {
	size_t end = filename.find_last_of(".");
	size_t start = filename.find_last_of("/\\");

	if (start == std::string::npos) start = 0;

	return filename.substr(start, end - start);
}

typedef void (*exec_class_fn)(meat::Reference, std::istream &);

/** Used to add functionallity to the Class subClass method when the compiler
 * is initialized. If the compiler is not initialized then the Class subClass
 * method should just raise an exception.
 */
static void classbuilder_int(meat::Reference super,
														 const std::string &cls_name,
                             const std::string &cls_body,
														 meat::Reference context) {
	meat::data::Library *grinder = meat::data::Library::get("Grinder");
	exec_class_fn exec_class =
		(exec_class_fn)grinder->dlsymbol("exec_class");

	meat::Reference ctx =
		meat::message(meat::Class::resolve("Grinder.Class"),
									"subClass:as:",
									context);
	meat::cast<meat::Context>(ctx).parameter(0, super);
	meat::cast<meat::Context>(ctx).parameter(1, new meat::Text(cls_name));
	meat::Reference cb = meat::execute(ctx);

	ctx = meat::message(library, "addClass:", context);
	meat::cast<meat::Context>(ctx).parameter(0, cb);
	execute(ctx);

  std::string body(cls_body);
  std::istringstream is(body);

	exec_class(cb, is);
}

/** Used for the Library import method when the compiler is initialized.
 */
static void compiler_import(const std::string &name,
														meat::Reference context) {
	meat::Reference new_ctx = message(library, "import:", context);
	meat::cast<meat::Context>(new_ctx).parameter(0, new meat::Text(name));
	execute(new_ctx);
}

/*********************
 * build_archive_app *
 *********************/

typedef void * interp_t;
typedef interp_t (*create_interp_fn)(const std::string &);
typedef void (*exec_interp_fn)(interp_t, std::istream &);
typedef void (*close_interp_fn)(interp_t);

static void build_archive_app() {
  meat::data::Library *grinder = meat::data::Library::get("Grinder");

	create_interp_fn create_interpreter =
		(create_interp_fn)grinder->dlsymbol("create_interpreter");
	exec_interp_fn exec_interpreter =
		(exec_interp_fn)grinder->dlsymbol("exec_interpreter");
	close_interp_fn close_interpreter =
		(close_interp_fn)grinder->dlsymbol("close_interpreter");

	interp_t interp = create_interpreter(out_file);

	// Go through each file and compile them
	for (std::deque<const char *>::iterator it = source_files.begin();
			 it != source_files.end();
			 ++it) {

#ifdef DEBUG
		std::cout << "DEBUG: Compiling " << *it << std::endl;
#endif /* DEBUG */

		std::ifstream meat_file;

		meat_file.open(*it, std::ios::in);

		if (meat_file.is_open()) {
			exec_interpreter(interp, meat_file);
			meat_file.close();
		} else {
			std::cerr << "ERROR: Unable to open " << *it << std::endl;
			return;
		}
	}

	close_interpreter(interp);
}

/*********************
 * build_library_app *
 *********************/

typedef void (*exec_library_fn)(meat::Reference, std::istream &);

static void build_library_app() {
	meat::data::Library *grinder = meat::data::Library::get("Grinder");
	exec_library_fn exec_library =
		(exec_library_fn)grinder->dlsymbol("exec_library");

	// Go through each file and compile them
	for (std::deque<const char *>::iterator it = source_files.begin();
			 it != source_files.end();
			 ++it) {

#ifdef DEBUG
		std::cout << "DEBUG: Compiling " << *it << std::endl;
#endif /* DEBUG */

		int ftype = meat::data::fl_type(*it);
		if (ftype == meat::data::FL_ARCHIVE) {
			meat::data::Archive arch(*it);
			meat::Reference obj = arch.get_object();

			if (obj->is_type("Grinder.Library")) {
				library = obj;
				break;
			} else {
			}

		} else {
			meat::Reference context =
				meat::message(meat::Class::resolve("Grinder.Library"),
											"new:", meat::Null());
			meat::cast<meat::Context>
				(context).parameter(0, new meat::Text(library_name(*it)));
			library = meat::execute(context);

			std::ifstream meat_file;

			meat_file.open(*it, std::ios::in);

			if (meat_file.is_open()) {
				exec_library(library, meat_file);
				meat_file.close();
			} else {
				std::cerr << "ERROR: Unable to open " << *it << std::endl;
				return;
			}
		}

		if (not app_class.empty()) {
			meat::Reference context = message(library, "setApplicationClass:",
																				meat::Null());
			meat::cast<meat::Context>(context).parameter(0, new meat::Text(app_class));
			meat::execute(context);
		}

		meat::Reference context = message(library, "compile", meat::Null());
		meat::execute(context);
	}

	library = NULL;
}

/******************************************************************************
 * Meat Ginder Entry
 */
int main(int argc, const char *argv[]) {

  /* The compiler state specified by the command line options. */
  static enum {
    BUILD_LIBRARY, INTREPRETER
  } state = BUILD_LIBRARY;

#ifdef DEBUG
  std::cout << "Grinder the Meat Compiler" << std::endl;
  std::cout << "  Debugging enabled" << std::endl;
#ifdef TESTING
  std::cout << "  Testing Build" << std::endl;
#endif
#endif

  meat::initialize(argc, argv, compiler_import, classbuilder_int);

#ifdef TESTING
  meat::test::run_tests();
#endif

  /*********************************
   * Parse the command line options.
   */
	int opt;
	while ((opt = getopt(argc, argv, "i:sc:#:h?")) != -1) {
		switch (opt) {
		case '#': // Display a hash value used in the vtables.
			std::cout << itohex(hash(optarg)) << std::endl;
			return 0;
		case 'c':
			app_class = optarg;
			break;
		case 'i':
			meat::data::Library::add_path(optarg);
			break;
		case 's':
			state = INTREPRETER;
			break;
		default: { // Unknown option
			std::cerr << "FATAL: unknown option -" << (char)opt << std::endl;
			return 1;
		}
		}
	}

	for (int c = optind; c < argc; c++)
		source_files.push_back(argv[c]);

	/***************************
   * Compile the meat files.
   */
  try{
    // Make sure we have files to compile
    if (source_files.size() == 0)
      throw meat::Exception("No source files specified to compile");

    meat::data::Library::import("Grinder");

		switch (state) {
		case BUILD_LIBRARY:
			build_library_app();
			break;
		case INTREPRETER:
			build_archive_app();
			break;
		default:
			throw meat::Exception("Options -l or -a were not specified, "
														"I don't know what to compile");
			break;
		}
  } catch (std::exception &err) {
    library = NULL;
    std::cerr << "UNCAUGHT EXCEPTION: " << err.what() << std::endl;

#ifdef TESTING
    meat::test::summary();
#endif

    return 1;
  }

#ifdef TESTING
  meat::test::summary();
#endif

  return 0;
}
