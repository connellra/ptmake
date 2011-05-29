#include <string>
#include <list>
#include <iostream>
#include <algorithm>
#include <gcrypt.h>
#include "re.h"
#include "file.h"
#include "rules.h"
#include "build.h"
#include "debug.h"
#include "parse.h"
#include "exception.h"
#include "subprocess.h"
#include "dependencies.h"

using namespace std;

// This is just for controlling the indentation of debug prints
int indentation = 0;

// List of all active rules
list<Rule *> Rule::rules;

void Rule::print()
{
	cout << "Rule:" << endl;

	if( targets != NULL ) {
		cout << "Targets:" << endl;
		for(list<string>::iterator i = targets->begin();
				i != targets->end();
				i ++)
		{
			cout << (*i);
		}
		cout << endl;
	}
	if( commands != NULL ) {
		cout << "Commands:" << endl;
		for(list<string>::iterator i = commands->begin();
				i != commands->end();
				i ++ )
		{
			cout << (*i) << endl;
		}
		cout << endl;
	}
}

void indent()
{
	int i;
	for( i = 0; i < indentation; i ++ ) {
		cout << " ";
	}
}

void print(std::string filename)
{
	indent();
	cout << "Depends on " << filename << endl;
}

void print(std::string filename, int status)
{
	indent();
	cout << "Depends on " << filename << "(" << status << ")" << endl;
}

void Rule::callback_entry(std::string filename)
{
	if( get_debug_level( DEBUG_DEPENDENCIES ) ) {
		::print(filename);
	}
	try {
		Rule *r = Rule::find(filename);
		r->execute( filename );
		dependencies.insert( pair<string,bool>(filename, true) );
	} catch( wexception &e ) {
		// Do nothing
	}
}

void Rule::callback_exit(std::string filename, bool success)
{
	dependencies.insert( pair<string,bool>(filename, success) );

	if( get_debug_level( DEBUG_DEPENDENCIES ) ) {
		::print(filename, success);
	}
}

bool Rule::execute(const std::string &target)
{
	unsigned char hash[32];
	bool needsRebuild = false;
	list<pair<string, bool> > *deps;

	// See if it's already being built
	if( built( target ) ) return false;
	// FIXME In a rule with multiple outputs, this should add everything
	// that was built to buildCache
	buildCache.insert( target );

	if( targets == NULL || commands == NULL ) return false;
	try {
		targetTime = fileTime( target );
	} catch( ... ) {
		// No target time, so definitely rebuild
	}

	if( get_debug_level( DEBUG_DEPENDENCIES ) ) {
		indent();
		cout << "Try to build: " << *targets->begin() << "(" << target << "," << targetTime << ")" << endl;
	}
	indentation ++;

	recalcHash( target, hash );
	// See if we have dependencies in the database
	deps = retrieve_dependencies( hash );
	// If we know the dependencies, we may be able to avoid building. If we
	// don't know the dependencies, we definitely have to rebuild.
	if( deps != NULL ) {
		for( list<pair<string, bool> >::iterator i = deps->begin(); i != deps->end(); i ++ ) {
			needsRebuild |= checkDep( target, i->first, i->second, targetTime );
		}
		delete deps;

		if( !needsRebuild ) {
			return false;
		}
		if( get_debug_level( DEBUG_DEPENDENCIES ) ) {
			cout << "Dependency updated, must build" << endl;
		}
	} else {
		// We don't know the dependencies, have to
		// build
		if( get_debug_level( DEBUG_REASON ) ) {
			cout << "Dependencies unknown, must build" << endl;
		}
	}
	
	clear_dependencies( hash );
	for(list<string >::iterator i = commands->begin(); i != commands->end(); i ++ ) {
		trace( expand_command( *i, target ) );

		// Touch the targets in case something else updated last in the build process 
	}
	add_dependencies( hash, dependencies );
	dependencies.clear();
	if( get_debug_level( DEBUG_DEPENDENCIES ) ) {
		indent();
		cout << "Done trying to build: " << *targets->begin() << "(" << target << "," << targetTime << ")" << endl;
	}
	indentation --;

	return true;
}

Rule::Rule( )
{
	rules.push_back(this);
	targets = NULL;
	declaredDeps = NULL;
	commands = NULL;
}

Rule *Rule::find(const string &target)
{
	Rule *r;
	bool foundARule = false;
	
	for(list<Rule *>::iterator i = rules.begin(); i != rules.end(); i ++ )
	{
		if( (*i)->match( target ) ) {
			if( foundARule ) {
				// We have multiple rules to build the target
				throw runtime_wexception( "Multiple rules" );
			}
			foundARule = true;
			r = *i;
		}
	}

	if( foundARule ) {
		return r;
	} else {
		throw runtime_wexception( "No rule" );
	}
}

bool Rule::match(const std::string &target)
{
	std::list<std::string>::iterator b, e, te;

	if( targets == NULL ) return false;

	b = targets->begin();
	e = targets->end();
	for( te = b; te != e; te ++ ) {
		if( ::match( *te, target ) ) {
			return true;
		}
	}
	return false;
}

bool Rule::canBeBuilt(const std::string &file)
{
	// Check if the file already exists
	if( fileExists( file ) ) return true;

	// If the file doesn't exist, see if we can build it
	try {
		Rule::find( file );
		return true;
	} catch( wexception &e ) {
		return false;
	}
}

void Rule::addTarget(const std::string &target)
{
	if( targets == NULL ) {
		targets = new list<string>;
	}
	targets->push_back(target);
}

void Rule::addTargetList(std::list<std::string> *targetList)
{
	if( targets != NULL ) {
		delete targets;
	}

	targets = targetList;
}

void Rule::addDependency(const std::string &dependency)
{
	if( declaredDeps == NULL ) {
		declaredDeps = new list<string>;
	}
	declaredDeps->push_back(dependency);
}

void Rule::addDependencyList(std::list<std::string> *dependencyList)
{
	if( declaredDeps != NULL ) {
		delete declaredDeps;
	}

	declaredDeps = dependencyList;
}

void Rule::addCommand(const std::string &command)
{
	if( commands == NULL ) {
		commands = new list<string>;
	}
	commands->push_back(command);
}

void Rule::addCommandList(std::list<std::string> *commandList)
{
	if( commands != NULL ) {
		delete commands;
	}

	commands = commandList;
}

void Rule::setDefaultTargets(void)
{
	if( !rules.empty() ) {
		Rule *r = *rules.begin();
		if( r->targets == NULL ) throw runtime_wexception("First rule has no targets");
		for(list<string>::iterator i = r->targets->begin(); i != r->targets->end(); i ++ ) {
			set_target(*i);
		}
	}
}

string Rule::expand_command( const string &command, const string &target )
{
	return command;
}

void Rule::recalcHash(string target, unsigned char hash[32])
{
	gcry_md_hd_t hd;

	gcry_md_open( &hd, GCRY_MD_SHA256, 0);
	if( targets != NULL ) {
		for(list<string>::iterator i = targets->begin(); i != targets->end(); i ++ ) {
			gcry_md_write( hd, i->c_str(), i->size() );
		}
	}

	if (commands != NULL ) {
		for(list<string>::iterator i = commands->begin(); i != commands->end(); i ++ ) {
			gcry_md_write( hd, i->c_str(), i->size() );
		}
	}
	gcry_md_write( hd, target.c_str(), target.size() );
	gcry_md_final( hd );

	memcpy( hash, gcry_md_read(hd, 0), 32 );

	gcry_md_close( hd );
}

bool Rule::built( const string &target )
{
	return buildCache.find( target ) != buildCache.end();
}

bool Rule::checkDep( const string &ruleTarget, const string &target, bool exists, time_t targetTime )
{
	// If the file has a rule, we need to try to rebuild it, and rebuild if that
	// succeeds.
	// If the file doesn't have a rule, then:
	// If the file didn't exist before, and it still doesn't, we don't need to
	// rebuild.
	// If the file didn't exist before, and it does now, we need to rebuild.
	// If the file existed before and doesn't exist now, we need to rebuild.
	// If the file existed before and still exists, we need to rebuild if the
	// file is newer
	if( get_debug_level( DEBUG_DEPENDENCIES ) ) {
		indent();
		cout << "Dependency " << target << "(" << exists << ")" << endl;
	}
	try {
		// Use a rule to rebuild
		Rule *r = Rule::find(target);
		if( r->execute( target ) ) {
			// It was rebuilt, so we need to rebuild the primary target
			return true;
		} else {
			bool status;
			time_t t;
			bool isDir;
			// If it wasn't rebuilt, but is already newer, we still
			// have to rebuild.
			status = fileTime(target, t, &isDir);
			if( !status || (t > targetTime && !isDir) ) {
				if( get_debug_level( DEBUG_REASON ) ) {
					indent();
					string t1 = ctime(&t);
					string t2 = ctime(&targetTime);
					t1 = t1.substr(0, t1.length() - 1);
					t2 = t2.substr(0, t2.length() - 1);
					cout << "Generated file out of date, need to rebuild " << ruleTarget << " because of " << target << ": (" << status << " ^ " << exists << ") || " << t1 << "(" <<  t <<  ") > " << t2 << "(" << targetTime << ")" << endl;
				}
				return true;
			}
		}
	} catch (...) {
		bool status;
		time_t t;
		bool isDir;

		status = fileTime(target, t, &isDir);
		if( (status ^ exists) || (status && t > targetTime && !isDir) ) {
			if( get_debug_level( DEBUG_REASON ) ) {
				indent();
				if( status && !exists ) {
					cout << "No rule to rebuild " << target << ": (new)" << endl;
				} else if( !status && exists ) {
					cout << "No rule to rebuild " << target << ": (deleted)" << endl;
				} else {
					string t1 = ctime(&t);
					string t2 = ctime(&targetTime);
					t1 = t1.substr(0, t1.length() - 1);
					t2 = t2.substr(0, t2.length() - 1);
					cout << "No rule to rebuild " << target << ": " << t1 << " (" << t << ")" << " > " << t2 << " (" << targetTime << ")" << endl;
				}
			}
			return true;
		}
	}

	return false;
}
