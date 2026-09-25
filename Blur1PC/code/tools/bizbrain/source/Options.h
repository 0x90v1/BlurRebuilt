#ifndef OPTIONS_H
#define OPTIONS_H

#ifdef _MSC_VER
#pragma once
#endif

struct C_Options
{
	Btl::String	user;
	Btl::String	pass;
	Btl::String	project;
	Btl::String	server;
	Btl::String	cmd;
	Btl::String	cmdArg;
};

#endif
