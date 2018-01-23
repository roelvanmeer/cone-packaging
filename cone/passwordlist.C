/* $Id: passwordlist.C,v 1.3 2003/06/05 02:54:23 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "gettext.H"
#include "passwordlist.H"
#include "myserver.H"
#include "myserverpromptinfo.H"
#include "curses/cursesstatusbar.H"
#include <unistd.h>

using namespace std;

extern CursesStatusBar *statusBar;

PasswordList PasswordList::passwordList;

PasswordList::PasswordList()
{
}

PasswordList::~PasswordList()
{
}

bool PasswordList::check(string url, string &pwd)
{
	string promptStr=_("Master Password (CTRL-C: cancel): ");

	// The following while() is really an if()

	while (!hasMasterPassword() && hasMasterPasswordFile())
	{
		myServer::promptInfo response=
			myServer::prompt(myServer::promptInfo(promptStr)
					 .password());

		if (response.abortflag)
			break;

		string p=response;

		if (p.size() == 0)
			break;

		if (myServer::loadpasswords(Gettext::toutf8(p)))
		{
			masterPassword=p;
			break;
		}

		promptStr=_("Invalid password, try again (CTRL-C: cancel): ");
		statusBar->beepError();
	}

	return get(url, pwd);
}

bool PasswordList::get(string url, string &pwd)
{
	map<string, string>::iterator p=list.find(url);

	if (p == list.end())
		return false;

	pwd= p->second;

	return true;
}

void PasswordList::save(string url, string pwd)
{
	remove(url);

	list.insert(make_pair(url, pwd));

	if (hasMasterPassword())
		myServer::savepasswords(masterPassword);
}

void PasswordList::remove(string url)
{
	map<string, string>::iterator p=list.find(url);

	if (p != list.end())
		list.erase(p);

	if (hasMasterPassword())
		myServer::savepasswords(masterPassword);
}

/////////////////////////////////////////////////////////////////////////////
//
// Master password support.

bool PasswordList::hasMasterPassword()
{
	return masterPassword.size() > 0;
}

bool PasswordList::hasMasterPasswordFile()
{
	string f=masterPasswordFile();

	return access(f.c_str(), R_OK) == 0;
}

void PasswordList::setMasterPassword(std::string newPassword)
{
	if (newPassword.size() == 0)
	{
		string n=masterPasswordFile();

		unlink(n.c_str());
	}
	else
	{
		myServer::savepasswords(newPassword);
	}

	masterPassword=newPassword;
}

void PasswordList::loadPasswords(const char * const *uids,
				 const char * const *pw)
{
	while (*uids && *pw)
	{
		list.insert(make_pair(string(*uids), string(*pw)));
		++uids;
		++pw;
	}
}


