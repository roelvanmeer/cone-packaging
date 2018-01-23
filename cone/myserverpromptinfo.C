/* $Id: myserverpromptinfo.C,v 1.3 2003/09/14 19:20:38 mrsam Exp $
**
** Copyright 2003, Double Precision Inc.
**
** See COPYING for distribution information.
*/

#include "config.h"
#include "gettext.H"
#include "myserverpromptinfo.H"
#include "curses/cursesfield.H"
#include "curses/cursesstatusbar.H"

using namespace std;

extern CursesStatusBar *statusBar;

myServer::promptInfo::promptInfo(string promptArg)
	: prompt(promptArg), value(""), isPassword(false),
	  isYesNo(false),
	  currentFocus(Curses::currentFocus), abortflag(false)
{
}

myServer::promptInfo::~promptInfo()
{
}

myServer::promptInfo &myServer::promptInfo::initialValue(string v)
{
	value=v;
	return *this;
}

myServer::promptInfo &myServer::promptInfo::password()
{
	isPassword=true;
	return *this;
}

myServer::promptInfo &myServer::promptInfo::yesno()
{
	isYesNo=true; return *this;
}

#if 0
myServer::promptInfo &myServer::promptInfo::option(wchar_t key, wchar_t keyL,
						   string keyname,
						   string keydescr)
{
	optionHelp.push_back(make_pair(keyname, keydescr));
	optionList.push_back(key);
	if (keyL)
		optionList.push_back(keyL);
	return *this;
}
#endif

myServer::promptInfo &myServer::promptInfo::option(Gettext::Key &theKey,
						   std::string keyname,
						   std::string keydescr)
{
	optionHelp.push_back(make_pair(keyname, keydescr));

	const vector<wchar_t> kv=theKey;

	optionList.insert(optionList.end(), kv.begin(), kv.end());
	return *this;

}

myServer::promptInfo myServer::prompt(myServer::promptInfo info)
{
	CursesField *f=statusBar->createPrompt(info.prompt,
					       info.value);

	if (!f)
	{
		info.abortflag=true;
		return info;
	}

	if (info.isPassword)
		f->setPasswordChar();

	if (info.isYesNo)
		f->setYesNo();

	if (info.optionHelp.size() > 0)
		f->setOptionHelp(info.optionHelp);

	if (info.optionList.size() > 0)
		f->setOptionField(info.optionList);

	//	f->setText(info.value);
	while (statusBar->prompting())
		eventloop();

	info.abortflag=statusBar->promptAborted();
	info.value=statusBar->getPromptValue();

	Curses::keepgoing=true;
	if (info.currentFocus)
		info.currentFocus->requestFocus();

	return info;
}
