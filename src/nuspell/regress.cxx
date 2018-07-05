/* Copyright 2018 Dimitrij Mijoski and Sander van Geloven
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file regress.cxx
 * Command line spell check regression testing.
 */

#include "dictionary.hxx"
#include "finder.hxx"
#include "string_utils.hxx"
#include <clocale>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <string>
#include <unordered_map>

#include <boost/locale.hpp>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#else
// manually define
#ifndef PACKAGE_STRING
#define PACKAGE_STRING "regress 2.0.0"
#endif
#ifndef PACKAGE
#define PACKAGE "regress"
#endif
#endif

#if defined(__MINGW32__) || defined(__unix__) || defined(__unix) ||            \
    (defined(__APPLE__) && defined(__MACH__))
#include <getopt.h>
#include <unistd.h>
#endif

#include "../hunspell/hunspell.hxx"

#ifdef _WIN32
#include <intrin.h>
uint64_t rdtsc() { return __rdtsc(); }

//  Linux/GCC
#else
uint64_t rdtsc()
{
	unsigned int lo, hi;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}
#endif

using namespace std;
using namespace nuspell;

enum Mode {
	DEFAULT_MODE /**< regression test */,
	HELP_MODE /**< printing help information */,
	VERSION_MODE /**< printing version information */,
	ERROR_MODE /**< where the arguments used caused an error */
};

struct Args_t {
	Mode mode = DEFAULT_MODE;
	string program_name = PACKAGE;
	string dictionary;
	string encoding;
	vector<string> other_dicts;
	vector<string> files;

	Args_t() {}
	Args_t(int argc, char* argv[]) { parse_args(argc, argv); }
	auto parse_args(int argc, char* argv[]) -> void;
};

auto Args_t::parse_args(int argc, char* argv[]) -> void
{
	if (argc != 0 && argv[0] && argv[0][0] != '\0')
		program_name = argv[0];
// See POSIX Utility argument syntax
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html
#if defined(_POSIX_VERSION) || defined(__MINGW32__)
	int c;
	// The program can run in various modes depending on the
	// command line options. mode is FSM state, this while loop is FSM.
	const char* shortopts = ":d:i:hv";
	const struct option longopts[] = {
	    {"version", 0, 0, 'v'},
	    {"help", 0, 0, 'h'},
	    {nullptr, 0, 0, 0},
	};
	while ((c = getopt_long(argc, argv, shortopts, longopts, nullptr)) !=
	       -1) {
		switch (c) {
		case 'd':
			if (dictionary.empty())
				dictionary = optarg;
			else
				cerr << "WARNING: Detected not yet supported "
				        "other dictionary "
				     << optarg << '\n';
			other_dicts.emplace_back(optarg);

			break;
		case 'i':
			encoding = optarg;

			break;
		case 'h':
			if (mode == DEFAULT_MODE)
				mode = HELP_MODE;
			else
				mode = ERROR_MODE;

			break;
		case 'v':
			if (mode == DEFAULT_MODE)
				mode = VERSION_MODE;
			else
				mode = ERROR_MODE;

			break;
		case ':':
			cerr << "Option -" << static_cast<char>(optopt)
			     << " requires an operand\n";
			mode = ERROR_MODE;
			break;
		case '?':
			cerr << "Unrecognized option: '-"
			     << static_cast<char>(optopt) << "'\n";
			mode = ERROR_MODE;
			break;
		}
	}
	files.insert(files.end(), argv + optind, argv + argc);
#endif
}

/**
 * Prints help information.
 * @param program_name pass argv[0] here.
 * @param out stream, standard output by default.
 */
auto print_help(const string& program_name) -> void
{
	auto& p = program_name;
	auto& o = cout;
	o << "Usage:\n"
	     "\n";
	o << p << " [-d dict_NAME] [-i enc] [file_name]...\n";
	o << p << " -h|--help|-v|--version\n";
	o << "\n"
	     "Regression testing spell check of each FILE. Without FILE, check "
	     "standard "
	     "input.\n"
	     "\n"
	     "  -d di_CT      use di_CT dictionary. Only one dictionary is\n"
	     "                currently supported\n"
	     "  -i enc        input encoding, default is active locale\n"
	     "  -h, --help    display this help and exit\n"
	     "  -v, --version print version number and exit\n"
	     "\n";
	o << "Example: " << p << " -d en_US file.txt\n";
	o << "\n"
	     "All word for which resutls differ with Hunspell are printed\n"
	     "standard output. At the end of each presented file, space-\n"
	     "separated statistics are printed to standard output, being:\n"
	     "total number of words\n"
	     "total CPU time for Nuspell\n"
	     "speedup factor compared to Hunspell\n"
	     "total true positives\n"
	     "true positive rate\n"
	     "total true negatives\n"
	     "true negative rate\n"
	     "total false positives\n"
	     "false positive rate\n"
	     "total false negatives\n"
	     "false negative rate\n";
	o << "\n"
	     "Bug reports: <https://github.com/hunspell/nuspell/issues>\n"
	     "Full documentation: "
	     "<https://github.com/hunspell/hunspell/wiki>\n"
	     "Home page: <http://hunspell.github.io/>\n";
}

/**
 * Prints the version number to standard output.
 */
auto print_version() -> void
{
	cout << PACKAGE_STRING
	    "\n"
	    "Copyright (C) 2018 Dimitrij Mijoski and Sander van Geloven\n"
	    "License LGPLv3+: GNU LGPL version 3 or later "
	    "<http://gnu.org/licenses/lgpl.html>.\n"
	    "This is free software: you are free to change and "
	    "redistribute it.\n"
	    "There is NO WARRANTY, to the extent permitted by law.\n"
	    "\n"
	    "Written by Dimitrij Mijoski, Sander van Geloven and others,\n"
	    "see https://github.com/hunspell/nuspell/blob/master/AUTHORS\n";
}

auto normal_loop(istream& in, ostream& out, Dictionary& dic, Hunspell& hun,
                 locale& hloc)
{
	auto word = string();
	// total number of words
	auto total = 0;
	// total number of words with identical spelling correctness
	auto truepos = 0;
	auto trueneg = 0;
	auto falsepos = 0;
	auto falseneg = 0;
	// store cpu time for Hunspell and Nuspell
	auto htime = rdtsc();
	htime = 0;
	auto ntime = htime;
	while (in >> word) {
		auto ticka = rdtsc();
		auto res = dic.spell(word, in.getloc());
		auto tickb = rdtsc();
		auto hres =
		    hun.spell(to_narrow(to_wide(word, in.getloc()), hloc));
		htime += tickb - ticka;
		ntime += rdtsc() - tickb;
		if (hres)
			if (res) {
				++truepos;
			}
			else {
				++falseneg;
				cout << word << '\n';
			}
		else if (res) {
			++falsepos;
			cout << word << '\n';
		}
		else {
			++trueneg;
		}
		++total;
	}
	// prevent drowning cerr in cout
	cout.flush();
	// prevent devision by zero
	if (total == 0) {
		clog << total << endl;
		return;
	}
	// rates
	auto speedup = (float)htime / ntime;
	auto trueposr = (float)truepos / total;
	auto truenegr = (float)trueneg / total;
	auto falseposr = (float)falsepos / total;
	auto falsenegr = (float)falseneg / total;

	clog << total << ' ' << ntime << ' ' << setprecision(3) << speedup
	     << ' ' << truepos << ' ' << setprecision(3) << trueposr << ' '
	     << trueneg << ' ' << setprecision(3) << truenegr << ' ' << falsepos
	     << ' ' << setprecision(3) << falseposr << ' ' << falseneg << ' '
	     << setprecision(3) << falsenegr << endl;
}

namespace std {
ostream& operator<<(ostream& out, const locale& loc)
{
	if (has_facet<boost::locale::info>(loc)) {
		auto& f = use_facet<boost::locale::info>(loc);
		out << "name=" << f.name() << ", lang=" << f.language()
		    << ", country=" << f.country() << ", enc=" << f.encoding();
	}
	else {
		out << loc.name();
	}
	return out;
}
} // namespace std

int main(int argc, char* argv[])
{
	// May speed up I/O. After this, don't use C printf, scanf etc.
	ios_base::sync_with_stdio(false);

	auto args = Args_t(argc, argv);
	if (args.mode == ERROR_MODE) {
		cerr << "Invalid (combination of) arguments, try '"
		     << args.program_name << " --help' for more information\n";
		return 1;
	}
	boost::locale::generator gen;
	auto loc = gen("");
	install_ctype_facets_inplace(loc);
	if (args.encoding.empty()) {
		cin.imbue(loc);
	}
	else {
		try {
			auto loc2 = gen("en_US." + args.encoding);
			install_ctype_facets_inplace(loc2);
			cin.imbue(loc2);
		}
		catch (const boost::locale::conv::invalid_charset_error& e) {
			cerr << e.what() << '\n';
#ifdef _POSIX_VERSION
			cerr << "Nuspell error: see `locale -m` for supported "
			        "encodings.\n";
#endif
			return 1;
		}
	}
	cout.imbue(loc);
	cerr.imbue(loc);
	clog.imbue(loc);
	setlocale(LC_CTYPE, "");

	switch (args.mode) {
	case HELP_MODE:
		print_help(args.program_name);
		return 0;
	case VERSION_MODE:
		print_version();
		return 0;
	default:
		break;
	}
	clog << "INFO: Input  locale " << cin.getloc() << '\n';
	clog << "INFO: Output locale " << cout.getloc() << '\n';

	auto f = Finder();
	f.add_default_paths();
	f.add_libreoffice_paths();
	f.add_mozilla_paths();
	f.add_apacheopenoffice_paths();
	f.search_dictionaries();

	if (args.dictionary.empty()) {
		// infer dictionary from locale
		auto& info = use_facet<boost::locale::info>(loc);
		args.dictionary = info.language();
		auto c = info.country();
		if (!c.empty()) {
			args.dictionary += '_';
			args.dictionary += c;
		}
	}
	auto filename = f.get_dictionary(args.dictionary);
	if (filename.empty()) {
		if (args.dictionary.empty())
			cerr << "No dictionary provided\n";
		else
			cerr << "Dictionary " << args.dictionary
			     << " not found\n";

		return 1;
	}
	clog << "INFO: Pointed dictionary " << filename << ".{dic,aff}\n";
	auto dic = Dictionary();
	try {
		dic = Dictionary::load_from_aff_dic(filename);
	}
	catch (const std::ios_base::failure& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	auto haff = filename + ".aff";
	auto hdic = filename + ".dic";
	Hunspell hun(haff.c_str(), hdic.c_str());
	auto hloc = gen("en_US"s + hun.get_dic_encoding());
	auto loop_function = normal_loop;

	if (args.files.empty()) {
		loop_function(cin, cout, dic, hun, hloc);
	}
	else {
		for (auto& file_name : args.files) {
			ifstream in(file_name.c_str());
			if (!in.is_open()) {
				cerr << "Can't open " << file_name << '\n';
				return 1;
			}
			in.imbue(cin.getloc());
			loop_function(in, cout, dic, hun, hloc);
		}
	}
	return 0;
}