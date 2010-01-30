#include <string>
#include <list>
#include <iostream>
#include <rules.h>

using namespace std;

struct _String {
    string *s;
};

struct _StringList {
    list<String *> sl;
};

struct _Dependencies {
	StringList *regular;
	StringList *orderOnly;
};

struct _RuleHeader {
	StringList *targetlist;
	Dependencies *dependencies;
};

struct _Rule {
	RuleHeader *header;
	StringList *commands;
};

void print_rule(Rule *rule)
{
	cout << "Rule:" << endl;
	cout << "Targets:" << endl;

	for(list<String *>::iterator i = rule->header->targetlist->sl.begin();
			i != rule->header->targetlist->sl.end();
			i ++)
	{
	}
	cout << endl;
	cout << "Dependencies:" << endl;
	for(list<String *>::iterator i = rule->header->targetlist->sl.begin();
			i != rule->header->targetlist->sl.end();
			i ++)
	{
	}
	cout << endl;
}

Rule *make_rule(RuleHeader *ruleHeader, StringList *commands)
{
	Rule *rule = new Rule;
	rule->header = ruleHeader;
	rule->commands = commands;

	return rule;
}

RuleHeader *make_rule_header(StringList *targetlist, Dependencies *dependencies)
{
	RuleHeader *ruleHeader = new RuleHeader;
	ruleHeader->targetlist = targetlist;
	ruleHeader->dependencies = dependencies;

	return ruleHeader;
}

Dependencies *make_dependencies(StringList *regular, StringList *orderOnly)
{
	Dependencies *dep = new Dependencies;

	dep->regular = regular;
	dep->orderOnly = orderOnly;

	return dep;
}

StringList *new_stringlist()
{
    return new StringList;
}

StringList *add_stringlist(StringList *stringlist, String *string)
{
	stringlist->sl.push_back( string );

	return stringlist;
}