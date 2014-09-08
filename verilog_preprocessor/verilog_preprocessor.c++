/* TODOS:
 * - find constants in not base 12 that have implicit length - add 32 to front
 * - convert multidim wire thing to use pure indicies
 * - generate loops
 * - add #(...) to defparam conversion
 * - remove signed, arithmetic shifts?
 * - use WireInfo in module reclaration
 *    also, make sure WireInfo handles spaces (or lack thereof) ...
 * - fix module rewrite detector (input with space thing?)
 * - output wire... put this, and others, like signed and input wire in a final touches
 *    pass. should simplify things.
 *   Lain
 */

#include <iostream>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cctype>
#include <deque>
#include <stack>
#include <math.h>
#include <stdexcept>

using namespace std;

class Macro {
public:
	Macro(istream& is);
	Macro(string name, const vector<string>& params, string body);
	string getName() { return name; }
	string expand(const vector<string>& args);
	bool isEmptyMacro() { return body.size() == 0; }
private:
	bool is_function_like;
	vector<string> params;
	string body;
	string name;
};

class WireInfo {
public:
	const string& getName() { return name; }
	const string& getType() { return type; }
	size_t getDimensionSize(size_t dim_number) {
		auto dim_info = dimension_sizes.at(dim_number-1);
		return dim_info.second - dim_info.first;
	};
	size_t getLowerBound(size_t dim_number) { return dimension_sizes.at(dim_number-1).first;  }
	size_t getUpperBound(size_t dim_number) { return dimension_sizes.at(dim_number-1).second; }
	size_t getNumDimensions() { return dimension_sizes.size(); }
	WireInfo()
		: name()
		, type()
		, use_custom_firstdim_decl(false)
		, custom_firstdim_decl()
		, dimension_sizes() { }
	string makeDeclaration();
	static std::pair<bool,WireInfo> parseWire(string&);
private:
	string name;
	string type;
	bool use_custom_firstdim_decl;
	string custom_firstdim_decl;
	vector<std::pair<size_t,size_t>> dimension_sizes;
};

void macro_expansion_pass(istream& is, ostream& os, const vector<string>& predef_macros);
void module_redeclaration_pass(istream& is, ostream& os);
void twodim_reduction_pass(istream& is, ostream& os);
void final_touches_pass(istream& is, ostream& os);

vector<string> parseParamList(istream& is);
vector<string> parseParamList(const string& params_string);

string readUntil(istream& from, const char* until, bool ignore_initial_whitespace);
vector<string> splitAndTrim(const string& s, char delim);
string& trim(string& str);
string trim(const string& str);
string skipToNextLineIfComment(char prev_char, char c, istream& is);

string generate_define(const string& params);

long mathEval(istream& expr);
long mathEval(const string& s) {
	istringstream iss(s);
	return mathEval(iss);
}

int main(int argc, char** argv) {
	vector<string> predef_macros;
	for (int i = 0; i < argc; ++i) {
		if (strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == 'D') {
			predef_macros.push_back(argv[i]+2);
		}
	}

	stringstream with_reduced_twodims;
	{
		stringstream with_redeclared_modules;
		{
			stringstream with_expanded_macros;
			{
				macro_expansion_pass(cin, with_expanded_macros, predef_macros);
			}
			module_redeclaration_pass(with_expanded_macros, with_redeclared_modules);
		}
		twodim_reduction_pass(with_redeclared_modules, with_reduced_twodims);
	}
	final_touches_pass(with_reduced_twodims,cout);
	return 0;
}

class IfdefState {
public:
	IfdefState() : in_disabled_ifdef_block(false), found_good_branch() {}
	void enterIfdef() { found_good_branch.push(false); }
	void exitIfdef() { found_good_branch.pop(); setInDisabledIfdefBlock(false); }
	void setInDisabledIfdefBlock(bool b) { in_disabled_ifdef_block = b; }
	bool getInDisabledIfdefBlock() { return in_disabled_ifdef_block; }
	bool foundGoodBranchAlready() { return found_good_branch.top(); }
	void setFoundGoodBranch(bool b) { found_good_branch.pop(); found_good_branch.push(b); }
private:
	bool in_disabled_ifdef_block;
	std::stack<bool> found_good_branch;
	IfdefState(const IfdefState&) = delete;
	IfdefState& operator=(const IfdefState&) = delete;
};

void macro_expansion_pass(istream& is, ostream& os, const vector<string>& predef_macros) {
	unordered_map<string,Macro> name2macro;
	for (
		auto predef_macro_name = predef_macros.begin();
		predef_macro_name != predef_macros.end();
		++predef_macro_name
	) {
		name2macro.insert(make_pair(*predef_macro_name, Macro(*predef_macro_name, {}, "")));
		os << "`define " << *predef_macro_name << "\n";
	}
	IfdefState ifdef_state{};

	char prev_char = '\0';
	while (true) {
		int c = is.get();
		if (is.eof()) {
			break;
		}

		string comment_line = skipToNextLineIfComment(prev_char,c,is);
		if (comment_line.size() > 0) {
			if (!ifdef_state.getInDisabledIfdefBlock()) {
				os << (char)c << comment_line;
			}
			c = is.get();
			string gendefine_flag = "%%GENDEFINE%%";
			if (comment_line.compare(0,gendefine_flag.size(),gendefine_flag) == 0) {
				string generated_define = generate_define(
					comment_line.substr(gendefine_flag.size())
				);
				istringstream generated_define_ss(generated_define);
				Macro m(generated_define_ss);
				name2macro.insert(make_pair(m.getName(),m));
				// cerr << "\n`define " << generated_define;
				// cerr << "generated `" << m.getName() << "'\n";
			}
		}
		if (is.eof()) {
			break;
		}

		if (c == '`') {
			string directive = trim(readUntil(is, ":;-+/*%){}[] (\n", true)); // arg.. regexes
			if (directive == "define" && !ifdef_state.getInDisabledIfdefBlock()) {
				Macro m(is);
				name2macro.insert(make_pair(m.getName(),m));
				if (m.isEmptyMacro()) {
					os << "`define " << m.getName() << '\n';
				}
			} else if (directive == "ifdef") {
				ifdef_state.enterIfdef();
				string test_name = trim(readUntil(is," \n",true));
				if (name2macro.find(test_name) != name2macro.end()) {
					// macro is defined
					ifdef_state.setFoundGoodBranch(true);
					ifdef_state.setInDisabledIfdefBlock(false);
				} else {
					ifdef_state.setInDisabledIfdefBlock(true);
				}
			} else if (directive == "elseif") {
				string test_name = trim(readUntil(is," \n",true));
				if (
					! ifdef_state.foundGoodBranchAlready()
					&& name2macro.find(test_name) != name2macro.end()) {
					ifdef_state.setInDisabledIfdefBlock(false);
					ifdef_state.setFoundGoodBranch(true);
				} else {
					ifdef_state.setInDisabledIfdefBlock(true);
				}
			} else if (directive == "else") {
				if (ifdef_state.foundGoodBranchAlready()) {
					ifdef_state.setInDisabledIfdefBlock(true);
				} else {
					ifdef_state.setInDisabledIfdefBlock(false);
					ifdef_state.setFoundGoodBranch(true);
				}
			} else if (directive == "endif") {
				ifdef_state.exitIfdef();
			} else if (!ifdef_state.getInDisabledIfdefBlock()) {
				auto lookup_result = name2macro.find(directive);
				if (lookup_result == name2macro.end()) {
					// string lines = readUntil(is,"\n",false);
					// lines += is.get();
					// lines += readUntil(is,"\n",false);
					// lines += is.get();
					// lines += readUntil(is,"\n",false);
					// lines += is.get();
					cerr
						<< "macro \""<<directive<<"\" has not been defined\n"
						// << "near " << lines << "\n"
						;
					exit(1);
				} else {
					vector<string> param_list;
					if (is.peek() == '(') {
						param_list = parseParamList(is);
					} // leave empty in other cases

					os << lookup_result->second.expand(param_list);
				}
			}
		} else if (!ifdef_state.getInDisabledIfdefBlock()) {
			os.put(c);
		}
		prev_char = c;
	}
}

void module_redeclaration_pass(istream& is, ostream& os) {
	int prev_char = ' ';
	while (true) {
		int c = is.get();
		if (is.eof()) {
			break;
		}

		string comment_line = skipToNextLineIfComment(prev_char,c,is);
		if (comment_line.size() > 0) {
			os.put(c);
			c = is.get();
			os << comment_line;
		}
		if (is.eof()) {
			break;
		}

		if (c == 'm' && isspace(prev_char)) {
			is.putback(c);
			string module_token;
			is >> module_token;
			os << module_token;
			if (module_token == "module") {
				string module_name = readUntil(is, "(", false);
				os << module_name;
				vector<string> module_params = parseParamList(is);
				vector<string> module_param_names;
				vector<string> module_param_types;

				os << '(';

				bool needs_redecl = false;
				for (auto param = module_params.begin(); param != module_params.end(); ++param) {
					if (param->find("input ") == 0 || param->find("output ") == 0) {
						needs_redecl = true;
						break;
					}
				}

				for (auto param = module_params.begin(); param != module_params.end(); ++param) {
					string::size_type last_space_index = param->find_last_of(" ");
					// (NOTE: whitespace is trimmed in parseParamList)
					if (needs_redecl) {
						string name = param->substr(last_space_index);
						os << name;
						module_param_names.push_back(name);
						module_param_types.push_back(param->substr(0, last_space_index));
					} else {
						os << *param;
					}
					if ((param + 1) != module_params.end()) {
						os << ",\n";
					}

				}

				os << ')';

				{
					string rest_of_decl = readUntil(is,";",false);
					os << rest_of_decl << (char)is.get() << '\n';
				}

				if (needs_redecl) {
					for (size_t i = 0; i < module_params.size(); ++i) {
						string::size_type position_of_reg = string::npos;
						if (module_param_types[i].find("output") != string::npos
							&& (position_of_reg = module_param_types[i].find("reg")) != string::npos) {
							// the case of an output reg
							string rest_of_type = module_param_types[i].substr(position_of_reg + 3);
							os << "output" << rest_of_type << module_param_names[i] << ";\n";
							os << "reg   " << rest_of_type << module_param_names[i] << ";\n";

						} else {
							os << module_params[i] << ";\n";
						}
					}
				}
			}
		} else {
			os.put(c);
		}
		prev_char = c;
	}
}

void twodim_reduction_pass_redecl(
	istream& is, ostream& os, unordered_map<string,WireInfo>& name2size);
void twodim_reduction_pass_rewrite(
	istream& is, ostream& os, unordered_map<string,WireInfo>& name2size);

void twodim_reduction_pass(istream& is, ostream& os) {
	unordered_map<string,WireInfo> name2size;
	stringstream with_redecl;
	twodim_reduction_pass_redecl(is, with_redecl, name2size);
	twodim_reduction_pass_rewrite(with_redecl, os, name2size);
}

void twodim_reduction_pass_redecl(
	istream& is, ostream& os, unordered_map<string,WireInfo>& name2size) {
	int prev_char = ' ';
	while (true) {
		int c = is.get();
		if (is.eof()) {
			break;
		}

		string comment_line = skipToNextLineIfComment(prev_char,c,is);
		if (comment_line.size() > 0) {
			os.put(c);
			c = is.get();
			os << comment_line;
		}
		if (is.eof()) {
			break;
		}

		if ((c == 'r' || c == 'w') && isspace(prev_char)) {
			is.putback(c);
			string decl;
			is >> decl;
			if (decl == "reg" || decl == "wire") {
				decl += readUntil(is, ";", false);
				trim(decl);
				bool success = false;
				WireInfo wire_info;
				std::tie(success,wire_info) = WireInfo::parseWire(decl);
				if (success && wire_info.getNumDimensions() > 1) {
					is.get(); // consume ';'
					name2size.insert(std::make_pair(wire_info.getName(),wire_info));
					os << wire_info.makeDeclaration();
				} else {
					os << decl;
				}
			} else {
				os << decl;
			}
		} else {
			os.put(c);
		}
		prev_char = c;
	}

	// cerr << "found twodims:\n";
	// for (auto twodim = name2size.begin(); twodim != name2size.end(); ++twodim) {
	// 	cerr << "name="<<twodim->first<<" range="<<twodim->second.first<<"-"<<twodim->second.second<<"\n";
	// }
}

void twodim_reduction_pass_rewrite(
	istream& is,
	ostream& os,
	unordered_map<string,WireInfo>& name2size
) {

	unordered_multimap<size_t,string> length2name;
	size_t longest_name = 2;

	for (
		auto name_and_size = name2size.begin();
		name_and_size != name2size.end();
		++name_and_size
	) {
		length2name.insert(make_pair(name_and_size->first.size(),name_and_size->first));
		if (name_and_size->first.size() > longest_name) {
			longest_name = name_and_size->first.size();
		}
	}

	deque<char> last_few_chars;
	bool flush_buffer = false;

	while (true) {
		while (last_few_chars.size() < longest_name) {
			last_few_chars.push_back(is.get());
			if (is.eof()) {
				last_few_chars.pop_back();
				break;
			}
		}

		if (!is.eof()) {
			string comment_line = skipToNextLineIfComment(last_few_chars[0],last_few_chars[1],is);
			if (comment_line.size() > 0) {
				for (size_t i = 0; i < comment_line.size(); ++i) {
					last_few_chars.push_back(comment_line[i]);
				}
				flush_buffer = true;
				goto continue_and_ouput;
			}
		}

		if (is.eof()) {
			// flush buffer & exit
			while (!last_few_chars.empty()) {
				os.put(last_few_chars.front());
				last_few_chars.pop_front();
			}
			break;
		}


		// see if the last n chars match a declared twodim (of length n)
		{
			string found_match = "";
			// cerr << "comparing with `" << last_few_chars << "'\n";
			for (size_t i = 1; i <= longest_name; ++i) { // must iterate from smallest to largest
				auto range = length2name.equal_range(i);
				if (range.first != length2name.end()) {
					// have entries of this size
					// cerr << "looking at twodims of size "<< i << '\n';
					for (auto l2name_iter = range.first; l2name_iter != range.second; ++l2name_iter) {
						string& name = l2name_iter->second;
						// cerr << "\tlooking at `" << name << "'\n";
						bool found_divergence = false;
						auto last_char = last_few_chars.rbegin();
						for (
							auto char_in_name = name.rbegin();
							char_in_name != name.rend() && last_char != last_few_chars.rend();
							++char_in_name, ++last_char
						) {
							if (*char_in_name != *last_char) {
								found_divergence = true;
								break;
							}
						}
						if (!found_divergence) {
							found_match = name;
							// cerr << name << " matches\n";
						}
					}
				}
			}

			// sub in the match

			if (found_match.size() > 0) {
				string next_chars;
				while (isspace(is.peek())) {
					next_chars += is.get();
				}
				if (is.peek() == '[') {
					is.get(); // consume '['
					stringstream new_suffix;
					string inside_brackets = readUntil(is,"]",false);
					istringstream inside_brackets_ss(inside_brackets);
					int evaluated_insides;
					bool good = false;
					try {
						evaluated_insides = mathEval(inside_brackets_ss);
						good = true;
					} catch (const std::invalid_argument&) {
					} catch (const std::out_of_range&) {
					}

					if (!good) {
						last_few_chars.push_back('[');
						for (size_t i = 0; i < inside_brackets.size(); ++i) {
							last_few_chars.push_back(inside_brackets[i]);
						}
						flush_buffer = true;
						goto continue_and_ouput;
					}

					new_suffix << "_" << evaluated_insides << next_chars;
					is.get(); // consume ']';
					while (true) {
						char c = new_suffix.get();
						if (new_suffix.eof()) {break;}
						last_few_chars.push_back(c);
					}
					flush_buffer = true;
				} else {
					// didn't find a use. Shove it all back in
					for (auto next_char = next_chars.rbegin(); next_char != next_chars.rend(); ++next_char) {
						is.putback(*next_char);
					}
				}
			}
		}

		continue_and_ouput:
		while (
			last_few_chars.size() >= longest_name
			|| (flush_buffer && !last_few_chars.empty())
		) {
			os.put(last_few_chars.front());
			// cerr.put(last_few_chars.front());
			last_few_chars.pop_front();
		}
		flush_buffer = false;
	}

}

vector<std::pair<string,string>> ft_strings_to_find {
	{" signed ", " "},
	{"output wire", "output"},
	{"input wire", "input"},
	{"'h", "32'h"},
};

void final_touches_pass(istream& is, ostream& os) {

	size_t buffer_size = 0;
	unordered_multimap<size_t,string> length2name;

	for (
		auto string_to_find = ft_strings_to_find.begin();
		string_to_find != ft_strings_to_find.end();
		++string_to_find
	) {
		length2name.insert(make_pair(string_to_find->first.size(),string_to_find->first));
		if (string_to_find->first.size() > buffer_size) {
			buffer_size = string_to_find->first.size();
		}
	}

	deque<char> last_few_chars;
	bool flush_buffer = false;

	while (true) {
		while (last_few_chars.size() < buffer_size) {
			last_few_chars.push_back(is.get());
			if (is.eof()) {
				last_few_chars.pop_back();
				break;
			}
		}

		if (!is.eof()) {
			string comment_line = skipToNextLineIfComment(last_few_chars[0],last_few_chars[1],is);
			if (comment_line.size() > 0) {
				for (size_t i = 0; i < comment_line.size(); ++i) {
					last_few_chars.push_back(comment_line[i]);
				}
				flush_buffer = true;
				goto continue_and_ouput;
			}
		}

		if (is.eof()) {
			// flush buffer & exit
			while (!last_few_chars.empty()) {
				os.put(last_few_chars.front());
				last_few_chars.pop_front();
			}
			break;
		}


		// see if the last n chars match a declared twodim (of length n)
		{
			string found_match = "";
			// cerr << "comparing with `" << last_few_chars << "'\n";
			for (size_t i = 1; i <= buffer_size; ++i) { // must iterate from smallest to largest
				auto range = length2name.equal_range(i);
				if (range.first != length2name.end()) {
					// have entries of this size
					// cerr << "looking at twodims of size "<< i << '\n';
					for (auto l2name_iter = range.first; l2name_iter != range.second; ++l2name_iter) {
						string& name = l2name_iter->second;
						// cerr << "\tlooking at `" << name << "'\n";
						bool found_divergence = false;
						auto last_char = last_few_chars.rbegin();
						for (
							auto char_in_name = name.rbegin();
							char_in_name != name.rend() && last_char != last_few_chars.rend();
							++char_in_name, ++last_char
						) {
							if (*char_in_name != *last_char) {
								found_divergence = true;
								break;
							}
						}
						if (!found_divergence) {
							found_match = name;
							// cerr << name << " matches\n";
						}
					}
				}
			}
			string output_str = "";
			if (found_match == " signed ") {
				output_str = " "; // eat it
			} else if (found_match == "output wire") {
				output_str = "output";
			} else if (found_match == "input wire") {
				output_str = "input";
			}
			if (output_str.size() > 0) {
				// cerr << "found: " << found_match << " replacing with: " << output_str << "\n";
				for (size_t i = 0; i < found_match.size(); ++i) {
					last_few_chars.pop_back();
				}
				for (size_t i = 0; i < output_str.size(); ++i) {
					last_few_chars.push_back(output_str[i]);
				}
			}
		}

		continue_and_ouput:
		while (
			last_few_chars.size() >= buffer_size
			|| (flush_buffer && !last_few_chars.empty())
		) {
			os.put(last_few_chars.front());
			// cerr.put(last_few_chars.front());
			last_few_chars.pop_front();
		}
		flush_buffer = false;
	}
}

Macro::Macro(string name_, const vector<string>& params_, string body_)
	: is_function_like(params_.size() != 0)
	, params(params_)
	, body(body_)
	, name(name_) {

}


Macro::Macro(istream& is)
	: is_function_like(false)
	, params()
	, body()
	, name() {

	name = trim(readUntil(is, "\n (", true));

	char next_char = is.get();
	while(next_char == ' ') {next_char = is.get();}
	is.putback(next_char);
	if (next_char == '(') {
		// if the next char is a '(' then it is a function-like
		is_function_like = true;
	} else {
		// case of siple macro
		is_function_like = false;
	}

	if (is_function_like) {
		params = parseParamList(is);
	}

	bool found_backslash = false;
	string line;
	while (true) {
		line += readUntil(is,"\\\n", false);
		if (is.get() == '\\') {
			found_backslash = true;
		} else {
			line += '\n';
			body += line;
			line.clear();
			if (!found_backslash) {
				break;
			}
			found_backslash = false;
		}
	}

	trim(body);

	// cerr
	// <<	"found definition of macro `"<<name<<"'\n"
	// <<	"params = "
	// ;
	// for (auto& param : params) {
	// 	cerr << param << " ,";
	// }
	// cerr <<	"$\nbody = "<<body<<"\n";
}

string Macro::expand(const vector<string>& args) {
	if (args.size() != params.size()) {
		cerr <<
			"num given args ("<<args.size()<<") and expected params ("<<params.size()<<")"
			" differ for macro \""<<name<<"\"\n";
		exit(1);
	}

	string expanded_body = body; // copy the unexpanded body;

	for (size_t i = 0; i < params.size(); ++i) {
		size_t pos = 0;
		while ((pos = expanded_body.find(params[i], pos)) != std::string::npos) {
			expanded_body.replace(pos, params[i].length(), args[i]);
			pos += args[i].length();
		}
	}

	return expanded_body;
}

string WireInfo::makeDeclaration() {
	string onedim_base;
	{
		ostringstream onedim_builder;

		onedim_builder << ( (trim(getType()) == "input wire") ? "input" : getType() ) << " [";

		if (use_custom_firstdim_decl) {
			onedim_builder << custom_firstdim_decl;
		} else {
			onedim_builder << getUpperBound(1) << ":" << getLowerBound(1);
		}
		onedim_builder << "] " << getName();

		onedim_base = onedim_builder.str();
	}

	if (getNumDimensions() > 1) {
		ostringstream builder;
		for (size_t i = getLowerBound(2); i <= getUpperBound(2); ++i) {
			builder << onedim_base << "_" << i << ";\n";
		}
		return builder.str();
	} else {
		return onedim_base;
	}
}

std::pair<size_t,size_t> parseVectorDeclation(const string& decl) {
	pair<size_t,size_t> result;

	istringstream second_dim_decl(
		trim(decl)
	);

	result.first = mathEval(readUntil(second_dim_decl, ":", true));
	second_dim_decl.get(); // consume ':'
	result.second = mathEval(second_dim_decl);
	return result;
}

std::pair<bool,WireInfo> WireInfo::parseWire(string& decl) {
	// cerr << "parsing wire/reg: `" << decl << "'\n";

	bool success = false;
	WireInfo wire_info;
	trim(decl);

	vector<string::size_type> bracket_locations {};
	while(true) {
		size_t prev_location = 0;
		if (bracket_locations.size() != 0) {
			prev_location = bracket_locations.back();
		}
		string::size_type next_bracket_location = decl.find_first_of("[", prev_location + 1);
		if (next_bracket_location == string::npos) {
			break;
		} else {
			bracket_locations.push_back(next_bracket_location);
		}
	}

	string::size_type end_of_type = decl.find_first_of(" [", 0);

	if (end_of_type == string::npos) {
		success = false;
		goto skip_to_return;
	}

	wire_info.type = trim(decl.substr(0, end_of_type));

	if (bracket_locations.size() == 0) {
		// cerr << "is nodim\n";
		wire_info.dimension_sizes.push_back(make_pair(0,0));
		wire_info.name = trim(
			decl.substr(
				decl.find_last_of(" ]") + 1,
				string::npos
			)
		);
		success = false;
	} else {
		for (
			auto bracket_location = bracket_locations.begin();
			bracket_location != bracket_locations.end();
			++bracket_location
		) {
			string dim_decl = decl.substr(
				*bracket_location + 1,
				decl.find_first_of("]",*bracket_location) - (*bracket_location + 1)
			);
			try {
				std::pair<size_t,size_t> dim_pair = parseVectorDeclation(dim_decl);
				if (dim_pair.first > dim_pair.second) {
					std::swap(dim_pair.first, dim_pair.second);
				}
				wire_info.dimension_sizes.push_back(dim_pair);
			} catch (std::invalid_argument& e) {
				wire_info.use_custom_firstdim_decl = true;
				wire_info.custom_firstdim_decl = dim_decl;
			}
			// cerr << "dim\n";
		}
		string::size_type first_closing_bracket = decl.find_first_of("]", 0);
		string::size_type end_of_name = decl.find_first_of(" [", first_closing_bracket + 2);
		wire_info.name = trim(
			decl.substr(
				first_closing_bracket + 1,
				end_of_name - (first_closing_bracket + 1)
			)
		);
		success = true;
	}


	skip_to_return:

	// cerr
	// 	<< 	"parsed WireInfo = {\n"
	// 		"\tname = \"" << wire_info.getName() << "\",\n"
	// 		"\ttype = \"" << wire_info.getType() << "\",\n"
	// 		"\tnum_dims = " << wire_info.getNumDimensions() << ",\n"
	// ;
	// for (size_t i = 1; i <= wire_info.getNumDimensions(); ++i) {
	// 	cerr
	// 	<<	"\tdimension["<<i<<"] = ["
	// 		<< wire_info.getLowerBound(i) << ':' << wire_info.getUpperBound(i)
	// 	<< "],\n";
	// }
	// cerr << "}\n";

	return std::make_pair(success, wire_info);
}

vector<string> parseParamList(const string& params_string) {
	istringstream is(trim(params_string));
	return parseParamList(is);
}

vector<string> parseParamList(istream& is) {
	char first_char;
	is >> first_char;
	if (first_char != '(') {
		cerr << "param list doesn't start with a '(' ( is '"<<first_char<<"')\n";
		exit(1);
	}

	string param_list = readUntil(is,")",true);
	is.get(); // consume ')'

	// cerr << "param_list=\"" << param_list << "\"\n";

	return splitAndTrim(param_list, ',');
}

string readUntil(istream& from, const char* until, bool ignore_initial_whitespace) {
	string result;
	bool first_time = true;
	bool newline_in_search_set = strchr(until,'\n');
	while (true) {
		char c;
		if (first_time && ignore_initial_whitespace) {
			from >> c;
		} else {
			c = from.get();
		}
		if (from.eof()) {
			break;
		}
		if (strchr(until,c) != NULL || (newline_in_search_set && (c == '\r') ) ) {
			// found a matching char.
			if (c == '\r' && newline_in_search_set && from.peek() == '\n') {
				// do nothing; leave the \n in the stream
			} else {
				from.putback(c);
			}
			break;
		} else {
			result += c;
		}
		first_time = false;
	}
	return result;
}

vector<string> splitAndTrim(const string& s, char delim) {
	istringstream ss(s);
	vector<string> result;
	string token;
	while (getline(ss, token, delim)) {
		trim(token);
		result.push_back(token);
	}
	return result;
}

string& trim(string& str) {
	str.erase(str.find_last_not_of(" \n\r\t") + 1, string::npos);
	str.erase(0, str.find_first_not_of(" \n\r\t"));
	return str;
}

string trim(const string& str) {
	string result = str;
	return trim(result);
}

string skipToNextLineIfComment(char prev_char, char c, istream& is) {
	if (prev_char == '/' && c == '/') {
		return readUntil(is,"\n",false);
	}
	return "";
}

std::pair<size_t,size_t> parseRange(const vector<string>& params, size_t index1, size_t index2) {
	std::pair<size_t,size_t> range;
	try {
		range.first = mathEval(params[index1]);
		range.second = mathEval(params[index2]);
	} catch (const std::invalid_argument& e) {
		cerr
			<< "bad GENDEFINE param "<<(index1+1)<<" or "<<(index2+1)<<": '" << params[index1]
			<< "'' or '" << params[index2] << "'\n";
		exit(1);
	} catch (const std::out_of_range& e) {
		cerr
			<< "bad GENDEFINE param "<<(index1+1)<<" or "<<(index2+1)<<" (out of range): '" << params[index1]
			<< "'' or '" << params[index2] << "'\n";
		exit(1);
	}
	return range;
}

string parseAssignmentOp(const vector<string>& params, size_t index) {
	if (params[index] == "nonblocking") {
		return "<=";
	} else if (params[index] == "blocking") {
		return "=";
	} else {
		cerr
			<< "bad GENDEFINE param #"<<(index+1)<<": `"<<params[index]
			<<"' did you mean blocking or nonblocking?\n";
		exit(1);
		return "";
	}
}

enum class GendefineType : size_t {
	NONE = 0,
	CHOOSE_TO,
	CHOOSE_FROM,
	ALWAYS_LIST,
	MOD_OP
};

namespace std {
	template<>
	struct hash<GendefineType> {
		size_t operator()(const GendefineType& gt) const {
			return std::hash<size_t>()(static_cast<size_t>(gt));
		}
	};
}

const std::unordered_map<GendefineType,size_t> gendefine_argnums {
	{GendefineType::CHOOSE_TO,     3},
	{GendefineType::CHOOSE_FROM,   3},
	{GendefineType::ALWAYS_LIST,   2},
	{GendefineType::MOD_OP,        4},
};

const std::unordered_map<string,GendefineType> string2gendefine {
	{"choose_to", GendefineType::CHOOSE_TO},
	{"choose_from",   GendefineType::CHOOSE_FROM  },
	{"always_list",  GendefineType::ALWAYS_LIST  },
	{"mod_op",        GendefineType::MOD_OP       },
};

string generate_define(const string& params_string) {
	vector<string> params = parseParamList(params_string);

	GendefineType this_gendefine_type = GendefineType::NONE;
	{
		auto found_gendefine_type = string2gendefine.find(params[0]);
		if (found_gendefine_type == string2gendefine.end()) {
			cerr << "bad GENDEFINE type in : `" << params_string << "'\n";
			exit(1);
		} else {
			this_gendefine_type = found_gendefine_type->second;
		}
	}

	if (params.size() != (gendefine_argnums.find(this_gendefine_type)->second + 1)) {
		cerr<<"bad number of arguments to GENDEFINE: `" << params_string << "'\n";
		exit(1);
	}

	ostringstream builder;

	// make the name
	builder << params[0];
	for (size_t i = 1; i < params.size(); ++i) {
		builder << '_' << params[i];
	}

	switch (this_gendefine_type) {
		case GendefineType::CHOOSE_TO:
		case GendefineType::CHOOSE_FROM: {
			std::pair<size_t,size_t> range = parseRange(params,2,3);
			string assignment_op = parseAssignmentOp(params,1);

			string assign_to;
			string assign_from;
			bool assign_to_is_indexed = false;
			bool assign_from_is_indexed = false;
			if (this_gendefine_type == GendefineType::CHOOSE_TO) {
				builder
					<< "(index_expr, assign_to, expression) \\\n\tcase (index_expr) \\\n"
				;
				assign_to = "assign_to";
				assign_to_is_indexed = true;
				assign_from = "expression";
				assign_from_is_indexed = false;
			} else if (this_gendefine_type == GendefineType::CHOOSE_FROM) {
				builder
					<< "(index_expr, assign_to, assign_from) \\\n\tcase (index_expr) \\\n"
				;
				assign_to = "assign_to";
				assign_to_is_indexed = false;
				assign_from = "assign_from";
				assign_from_is_indexed = true;
			}

			for (size_t i = range.first; i <= range.second; ++i) {
				if (i == range.second) {
					builder << "\t\tdefault";
				} else {
					builder << "\t\t'd" << i;
				}
				builder << ':' << assign_to;
				if (assign_to_is_indexed) {
					builder << '[' << i << ']';
				}
				builder << ' ' << assignment_op << ' ' << assign_from;
				if (assign_from_is_indexed) {
					builder << '[' << i << ']';
				}
				builder << "; \\\n";
			}

			builder << "\tendcase";
		} break;
		case GendefineType::ALWAYS_LIST: {
			std::pair<size_t,size_t> range = parseRange(params,1,2);
			builder << "(wire_name) \\\n";
			for (size_t i = range.first; i <= range.second; ++i) {
				builder << "wire_name[" << i << "]";
				if (i != range.second) {
					builder << " or ";
				}
			}
		} break;
		case GendefineType::MOD_OP: {
			string assignment_op = parseAssignmentOp(params,1);
			auto range = parseRange(params,3,4);
			size_t mod_by = 1;
			try {
				mod_by = stoi(params[2]);
			} catch (const std::invalid_argument& e) {
				cerr << "bad GENDEFINE param #2 in `" << params_string << "'\n";
				exit(1);
			} catch (const std::out_of_range& e) {
				cerr << "bad GENDEFINE param #2 (out of range) in `" << params_string << "'\n";
				exit(1);
			}
			builder << "(input, output) \\\n\tcase (input) \\\n";
			for (size_t i = range.first; i <= range.second; ++i) {
				builder
					<< "\t\t'd" << i << ": output " << assignment_op << ' ' << (i % mod_by) << "; \\\n"
				;
			}
			builder << "\tendcase";
		} break;
		case GendefineType::NONE:
		break;
	}

	return builder.str();
}

/// algorithm from http://en.wikipedia.org/wiki/Operator-precedence_parser
// expression ::= primary OPERATOR primary
// primary ::= '(' expression ')' | NUMBER | VARIABLE

unordered_map<char,int> precedence_map = {
	{ '+' , 1 },
	{ '-' , 1 },
	{ '*' , 2 },
	{ '/' , 2 },
	{ '%' , 2 },
	{ '^' , 3 },
};

long expression(istream& is, long lhs, long min_precedence);
long parse_primary(istream& is);
long do_binary_op(long lhs, long rhs, char op);

long mathEval(istream& expr) {
	return expression(expr, parse_primary(expr), 0);
}

/**
 * Parse out a primary expression, something that is self-contained, and
 * has a value; a number, a parentheses enclosed expression, or a
 * constant (though the last one in currently unsupported)
 */
long parse_primary(istream& is) {
	// read until you've found something that definite ends,
	// or starts, (in the case of ')' ) a primary expression
	string primary = trim(readUntil(is, "+-*/%(^ ",true));
	long result = 0;
	if (is.peek() == '(') {
		if (primary.size() != 0) {
			cerr << "Bad primary. Unexpected `" <<primary<< "' in `" <<primary<<readUntil(is,"\n",false)<< "'\n";
			exit(1);
		}
		is.get(); // consume '('
		string subexpr = readUntil(is, ")", true);
		if (is.eof()) {cerr << "no matching ')'\n"; exit(1);}
		is.get(); // consume ')'m
		stringstream subexprss(trim(subexpr));
		result = expression(subexprss, parse_primary(subexprss), 0);
	} else {
		try {
			// TODO: parse variable names here
			result = stol(primary);
		} catch (const std::invalid_argument& e) {
			cerr << "bad long: " << primary << "\n"; throw e;
		} catch (const std::out_of_range& e) {
			cerr << "long out of range: " << primary << "\n"; throw e;
		}
	}
	return result;
}

// from http://en.wikipedia.org/wiki/Operator-precedence_parser
// parse_expression (lhs, min_precedence)
//     while the next token is a binary operator whose precedence is >= min_precedence
//         op := next token
//         rhs := parse_primary ()
//         while the next token is a binary operator whose precedence is greater
//                  than op's, or a right-associative operator
//                  whose precedence is equal to op's
//             lookahead := next token
//             rhs := parse_expression (rhs, lookahead's precedence)
//         lhs := the result of applying op with operands lhs and rhs
//     return lhs

long expression(istream& is, long lhs, long min_precedence) {
	while (true) {
		char this_op;
		if (!(is >> this_op)) {
			break;
		}
		long this_op_precedence = precedence_map.find(this_op)->second;
		if (this_op_precedence < min_precedence) {
			is.putback(this_op);
			return lhs;
		}
		long rhs = parse_primary(is);
		while (true) {
			char next_op = 0;
			if (!(is >> next_op)) {
				break;
			}
			// TODO: support right-associative operators (eg. unary ones)
			long next_op_precedence = precedence_map.find(next_op)->second;
			is.putback(next_op);
			if (next_op_precedence <= this_op_precedence) {
				break;
			}
			rhs = expression(is, rhs, next_op_precedence);
		}
		lhs = do_binary_op(lhs, rhs, this_op);
	}
	return lhs;
}

long do_binary_op(long lhs, long rhs, char op) {
	switch (op) {
		case '*': return lhs * rhs;
		case '/': return lhs / rhs;
		case '+': return lhs + rhs;
		case '-': return lhs - rhs;
		case '%': return lhs % rhs;
		case '^': return lround(pow(lhs,rhs));
		default :
			cerr << "bad operator : '" << op << "'\n";
			exit(1);
			return 0;
	}
}
