#include "stdafx.h"
#include "Display/Device/D3D9/PCD3D9CapsBuilder.h"

#ifdef _DEBUG
#define DEBUG_OUTPUT
#endif

//#define OUTPUT_TO_TXT_FILE
#pragma warning(disable:4530)

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <conio.h>

namespace Display
{
namespace D3D9
{

char buffer[2048];
char capsname[32];

void DBPrint(const char* text, ...)
{
	static char temp[512];

	va_list arglist;
	va_start(arglist, text);
	vsprintf_s(temp, 512, text, arglist);
	va_end(arglist);

	OutputDebugString(temp);
	
	std::cout << temp;
}

void DBError(const char* text, ...)
{
	static char temp1[512];
	static char temp2[512];

	va_list arglist;
	va_start(arglist, text);
	vsprintf_s(temp1, 512, text, arglist);
	va_end(arglist);

	sprintf_s(temp2, 512, "ERROR: %s", temp1);
	OutputDebugString(temp2);
	
	std::cerr << temp2;
}

void Write(std::ofstream& ofile, const char* text, ...)
{
	static char temp[512];

	va_list arglist;
	va_start(arglist, text);
	vsprintf_s(temp, sizeof(temp), text, arglist);
	va_end(arglist);

	ofile << temp;
}

void WriteCAPS(std::ofstream& ofile, std::string capsVarName, std::string capsDefName, const bool disabled)
{
#ifdef OUTPUT_TO_TXT_FILE
	Write(ofile, "%i %s %s\n",
		!disabled,
		capsVarName.c_str(),
		capsDefName.c_str());
#else
	const char* temp = disabled ? "//" : "";
	Write(ofile, "	%sif(!(d3dCaps.%s & %s))\n",
		temp,
		capsVarName.c_str(),
		capsDefName.c_str());
	Write(ofile, "	%s{\n",
		temp);
	Write(ofile, "		%sLOGINFO(g_Log, \"*** D3D%sCapsCheckFailure :  %s & %s\\n\");\n",
		temp,
		capsname,
		capsVarName.c_str(),
		capsDefName.c_str());
	//Write(ofile, "		%sreturn false;\n",
		//temp);
	Write(ofile, "		%ssupported = false;\n",
		temp);
	Write(ofile, "	%s}\n",
		temp);
#endif
}

void WriteDWORD(std::ofstream& ofile, std::string capsVarName, const DWORD value, const bool disabled)
{
#ifdef OUTPUT_TO_TXT_FILE
	Write(ofile, "%i %s\n",
		value,
		capsVarName.c_str());
#else
	const char* temp = disabled ? "//" : "";
	Write(ofile, "	%sif(d3dCaps.%s < %i)\n",
		temp,
		capsVarName.c_str(),
		value);
	Write(ofile, "	%s{\n",
		temp);
	Write(ofile, "		%sLOGINFO(g_Log, \"*** D3D%sCapsCheckFailure : %s (%%i) < %i\\n\", d3dCaps.%s);\n",
		temp,
		capsname,
		capsVarName.c_str(),
		value,
		capsVarName.c_str());
	//Write(ofile, "		%sreturn false;\n",
		//temp);
	Write(ofile, "		%ssupported = false;\n",
		temp);
	Write(ofile, "	%s}\n",
		temp);
#endif
}

void WriteUINT(std::ofstream& ofile, std::string capsVarName, const UINT value, const bool disabled)
{
#ifdef OUTPUT_TO_TXT_FILE
	Write(ofile, "%i %s\n",
		value,
		capsVarName.c_str());
#else
	const char* temp = disabled ? "//" : "";
	Write(ofile, "	%sif(d3dCaps.%s < %i)\n",
		temp,
		capsVarName.c_str(),
		value);
	Write(ofile, "	%s{\n",
		temp);
	Write(ofile, "		%sLOGINFO(g_Log, \"*** D3D%sCapsCheckFailure : %s (%%i) < %i\\n\", d3dCaps.%s);\n",
		temp,
		capsname,
		capsVarName.c_str(),
		value,
		capsVarName.c_str());
	//Write(ofile, "		%sreturn false;\n",
		//temp);
	Write(ofile, "		%ssupported = false;\n",
		temp);
	Write(ofile, "	%s}\n",
		temp);
#endif
}

void WriteINT(std::ofstream& ofile, std::string capsVarName, const INT value, const bool disabled)
{
#ifdef OUTPUT_TO_TXT_FILE
	Write(ofile, "%i %s\n",
		value,
		capsVarName.c_str());
#else
	const char* temp = disabled ? "//" : "";
	Write(ofile, "	%sif(d3dCaps.%s < %i)\n",
		temp,
		capsVarName.c_str(),
		value);
	Write(ofile, "	%s{\n",
		temp);
	Write(ofile, "		%sLOGINFO(g_Log, \"*** D3D%sCapsCheckFailure : %s (%%i) < %i\\n\", d3dCaps.%s);\n",
		temp,
		capsname,
		capsVarName.c_str(),
		value,
		capsVarName.c_str());
	//Write(ofile, "		%sreturn false;\n",
		//temp);
	Write(ofile, "		%ssupported = false;\n",
		temp);
	Write(ofile, "	%s}\n",
		temp);
#endif
}

void WriteFLOAT(std::ofstream& ofile, std::string capsVarName, const float value, const bool disabled)
{
#ifdef OUTPUT_TO_TXT_FILE
	Write(ofile, "%.2f %s\n",
		value,
		capsVarName.c_str());
#else
	const char* temp = disabled ? "//" : "";
	Write(ofile, "	%sif(d3dCaps.%s < %i)\n",
		temp,
		capsVarName.c_str(),
		value);
	Write(ofile, "	%s{\n",
		temp);
	Write(ofile, "		%sLOGINFO(g_Log, \"*** D3D%sCapsCheckFailure : %s (%%f) < %f\\n\", d3dCaps.%s);\n",
		temp,
		capsname,
		capsVarName.c_str(),
		value,
		capsVarName.c_str());
	//Write(ofile, "		%sreturn false;\n",
		//temp);
	Write(ofile, "		%ssupported = false;\n",
		temp);
	Write(ofile, "	%s}\n",
		temp);
#endif
}

void WriteD3DDEVTYPE(std::ofstream& ofile, std::string capsVarName, const D3DDEVTYPE value, const bool disabled)
{
#ifdef OUTPUT_TO_TXT_FILE
	Write(ofile, "%i %s\n",
		value,
		capsVarName.c_str());
#else
	const char* temp = disabled ? "//" : "";
	Write(ofile, "	%sif(d3dCaps.%s < %i)\n",
		temp,
		capsVarName.c_str(),
		value);
	Write(ofile, "	%s{\n",
		temp);
	Write(ofile, "		%sLOGINFO(g_Log, \"*** D3D%sCapsCheckFailure : %s (%%i) < %i\\n\", d3dCaps.%s);\n",
		temp,
		capsname,
		capsVarName.c_str(),
		value,
		capsVarName.c_str());
	//Write(ofile, "		%sreturn false;\n",
		//temp);
	Write(ofile, "		%ssupported = false;\n",
		temp);
	Write(ofile, "	%s}\n",
		temp);
#endif
}

int GetSizeFromType(const char* type)
{
	if(strcmp(type, "DWORD") == 0)
	{
		return sizeof(DWORD);
	}
	else
	if(strcmp(type, "UINT") == 0)
	{
		return sizeof(UINT);
	}
	else
	if(strcmp(type, "INT") == 0)
	{
		return sizeof(INT);
	}
	else
	if(strcmp(type, "float") == 0)
	{
		return sizeof(float);
	}
	else
	if(strcmp(type, "D3DDEVTYPE") == 0)
	{
		return sizeof(D3DDEVTYPE);
	}
	else
	{
		assert(0);
		return 0;
	}
}

struct CapsDef
{
	char						m_Name[128];
	char						m_Value[128];
};

struct C_CapsVar
{
	C_CapsVar() : m_Disable(false)
	{
	}

	~C_CapsVar()
	{
		m_DefsList.clear();
	}

	bool						m_Disable;
	char						m_Type[64];
	char						m_Name[64];
	char 						m_Caps[64];
	std::vector<CapsDef>		m_DefsList;
};

struct C_CapsType
{
	~C_CapsType()
	{
		m_VarList.clear();
	}

	int GetSize()
	{
		int size = 0;
		for(unsigned int i=0; i<m_VarList.size(); i++)
		{
			C_CapsVar& capsVar = m_VarList[i];

			size += GetSizeFromType(capsVar.m_Type);
		}
		return size;
	}

	int							m_EndPos;
	char						m_Name[64];
	std::vector<C_CapsVar>		m_VarList;
};

struct C_Caps
{
	~C_Caps()
	{
		m_TypeList.clear();
	}

	int GetOffset(const char* type)
	{
		for(unsigned int i=0; i<m_TypeList.size(); i++)
		{
			C_CapsType& capsType = m_TypeList[i];

			if(strcmp(type, capsType.m_Name) == 0)
			{
				return capsType.GetSize();
			}
		}

		return GetSizeFromType(type);
	}

	C_CapsType* GetType(const char* type)
	{
		for(unsigned int i=0; i<m_TypeList.size(); i++)
		{
			C_CapsType& capsType = m_TypeList[i];

			if(strcmp(type, capsType.m_Name) == 0)
			{
				return &capsType;
			}
		}

		return NULL;
	}

	std::vector<C_CapsType>		m_TypeList;
};

void ParseVars(std::ifstream& ifile, C_CapsVar& capsVar, C_CapsType& capsType)
{
	ifile >> buffer >> std::ws;

	if(buffer[strlen(buffer)-1] == ',')
	{
		strncpy_s(capsVar.m_Name, sizeof(capsVar.m_Name), buffer, strlen(buffer)-1);
		capsType.m_VarList.push_back(capsVar);
		
		ParseVars(ifile, capsVar, capsType);
	}
	else
	if(buffer[strlen(buffer)-1] == ';')
	{
		strncpy_s(capsVar.m_Name, sizeof(capsVar.m_Name), buffer, strlen(buffer)-1);
		capsType.m_VarList.push_back(capsVar);
	}
	else
	{
		assert(0);
	}
}

void ParseTypes(std::ifstream& ifile, C_Caps& caps)
{
	ifile.seekg(0, std::ios_base::beg);

	while(strcmp(buffer, "END_OF_FILE") != 0)
	{
		ifile >> buffer >> std::ws;

		if(strcmp(buffer, "struct") == 0)
		{
			C_CapsType capsType;

			ifile >> buffer >> std::ws;

			while(1)
			{
				ifile >> buffer;

				if(buffer[0] == '}')
				{
					ifile >> std::ws;
					break;
				}
				else
				if(buffer[0] == '{')
				{
					ifile >> std::ws;
				}
				else
				if(buffer[0] == '/' && buffer[1] == '*')
				{
					ifile.getline(buffer, sizeof(buffer));
				}
				else
				if(buffer[0] == '/' && buffer[1] == '/')
				{
					ifile >> std::ws >> buffer;

					if(strstr(buffer, "D3D"))
					{
						if(capsType.m_VarList.size() > 0)
						{
							C_CapsVar& capsVar = capsType.m_VarList.back();

							strcpy_s(capsVar.m_Caps, sizeof(capsVar.m_Caps), buffer);
							strcat_s(capsVar.m_Caps, sizeof(capsVar.m_Caps), "_");
						}
					}
					else
					if(strcmp(buffer, "DISABLE") == 0)
					{
						if(capsType.m_VarList.size() > 0)
						{
							C_CapsVar& capsVar = capsType.m_VarList.back();

							capsVar.m_Disable = true;
						}
					}

					ifile.getline(buffer, sizeof(buffer));
				}
				else
				{
					ifile >> std::ws;

					C_CapsVar capsVar;

					strcpy_s(capsVar.m_Type, sizeof(capsVar.m_Type), buffer);
					strcpy_s(capsVar.m_Caps, sizeof(capsVar.m_Caps), "");

					ParseVars(ifile, capsVar, capsType);
				}
			}

			ifile >> buffer >> std::ws;

			strncpy_s(capsType.m_Name, sizeof(capsType.m_Name), buffer, strlen(buffer)-1);

			capsType.m_EndPos = ifile.tellg();

			caps.m_TypeList.push_back(capsType);
		}
	}
}

void ParseDefs(std::ifstream& ifile, C_Caps& caps)
{
	C_CapsType& capsType = caps.m_TypeList.back();

	for(unsigned int i=0; i<capsType.m_VarList.size(); i++)
	{
		C_CapsVar& capsVar = capsType.m_VarList[i];

		if(strcmp(capsVar.m_Caps, "") != 0)
		{
			ifile.seekg(capsType.m_EndPos);

			while(1)
			{
				ifile >> buffer;

				if(strcmp(buffer, "END_OF_FILE") == 0)
				{
					break;
				}
				else
				if(buffer[0] == '/')
				{
					ifile.getline(buffer, sizeof(buffer));
				}
				else
				if(strstr(buffer, capsVar.m_Caps))
				{
					CapsDef capsDef;
					
					strcpy_s(capsDef.m_Name, sizeof(capsDef.m_Name), buffer);
					
					ifile  >> std::ws >> buffer >> std::ws;

					strcpy_s(capsDef.m_Value, sizeof(capsDef.m_Value), buffer);
					
					capsVar.m_DefsList.push_back(capsDef);
				}
			}
		}
	}
}

void OutputType(std::ofstream& ofile, C_CapsType& capsType, C_Caps& caps, const D3DCAPS9& d3dCaps, int& offset, std::string tag);

void OutputVar(std::ofstream& ofile, C_CapsVar& capsVar, C_Caps& caps, const D3DCAPS9& d3dCaps, int& offset, std::string tag)
{
	if(strcmp(capsVar.m_Type, "DWORD") == 0)
	{
		if(strcmp(capsVar.m_Caps, "") != 0)
		{
			for(unsigned int capsDefIndex=0; capsDefIndex<capsVar.m_DefsList.size(); capsDefIndex++)
			{
				CapsDef& capsDef = capsVar.m_DefsList[capsDefIndex];
				DWORD caps = *((DWORD*)(((BYTE*)&d3dCaps)+offset));
				unsigned long value = strtoul(capsDef.m_Value, NULL, 16);
				
				bool disabled = capsVar.m_Disable;
				/*if(!disabled)
				{
					if(!(caps & value))
					{
						disabled = true;
					}
				}*/

				WriteCAPS(ofile, tag + capsVar.m_Name, capsDef.m_Name, disabled);
			}
		}
		else
		{
			DWORD value = *((DWORD*)(((BYTE*)&d3dCaps)+offset));
			WriteDWORD(ofile, tag + capsVar.m_Name, value, capsVar.m_Disable);
		}

		offset += sizeof(DWORD);
	}
	else
	if(strcmp(capsVar.m_Type, "UINT") == 0)
	{
		UINT value = *((UINT*)(((BYTE*)&d3dCaps)+offset));
		WriteUINT(ofile, tag + capsVar.m_Name, value, capsVar.m_Disable);
		offset += sizeof(UINT);
	}
	else
	if(strcmp(capsVar.m_Type, "INT") == 0)
	{
		INT value = *((INT*)(((BYTE*)&d3dCaps)+offset));
		WriteINT(ofile, tag + capsVar.m_Name, value, capsVar.m_Disable);
		offset += sizeof(INT);
	}
	else
	if(strcmp(capsVar.m_Type, "float") == 0)
	{
		float value = *((float*)(((BYTE*)&d3dCaps)+offset));
		WriteFLOAT(ofile, tag + capsVar.m_Name, value, capsVar.m_Disable);
		offset += sizeof(float);
	}
	else
	if(strcmp(capsVar.m_Type, "D3DDEVTYPE") == 0)
	{
		D3DDEVTYPE value = *((D3DDEVTYPE*)(((BYTE*)&d3dCaps)+offset));
		WriteD3DDEVTYPE(ofile, tag + capsVar.m_Name, value, capsVar.m_Disable);
		offset += sizeof(D3DDEVTYPE);
	}
	else
	{
		C_CapsType* pCapsType = caps.GetType(capsVar.m_Type);
		if(pCapsType)
		{
			OutputType(ofile, *pCapsType, caps, d3dCaps, offset, tag + capsVar.m_Name + ".");
		}
		else
		{
			assert(0);
		}
	}
}

void OutputType(std::ofstream& ofile, C_CapsType& capsType, C_Caps& caps, const D3DCAPS9& d3dCaps, int& offset, std::string tag)
{
	for(unsigned int capsVarIndex=0; capsVarIndex<capsType.m_VarList.size(); capsVarIndex++)
	{
		C_CapsVar& capsVar = capsType.m_VarList[capsVarIndex];

		OutputVar(ofile, capsVar, caps, d3dCaps, offset, tag);
	}
}

bool D3DBuildCaps(const D3DCAPS9& d3dCaps, const char* ofilename)
{
	strcpy_s(capsname, sizeof(capsname), ofilename);

	*buffer = '\0';

	C_Caps caps;
	
	{
		const char* ifilepath = "d3d9caps.h";
		std::ifstream ifile;
		ifile.open(ifilepath);
		if(!ifile.is_open())
		{
			DBError("Failed to open file %s\n", ifilepath);
			return false;
		}

		ParseTypes(ifile, caps);

		ParseDefs(ifile, caps);

		ifile.close();
		ifile.clear();
	}

	{
		char ofilepath[1024];
		#ifdef OUTPUT_TO_TXT_FILE
		sprintf_s(ofilepath, sizeof(ofilepath), "D3D%sCaps.txt", ofilename);
		#else
		sprintf_s(ofilepath, sizeof(ofilepath), "Z:\\devroot\\code\\mercury\\library\\display\\source\\device\\d3d9\\PCD3D9CapsChecker%s.cpp", ofilename);
		#endif
		
		std::ofstream ofile;
		ofile.open(ofilepath);
		if(!ofile.is_open())
		{
			DBError("Failed to open output file %s\n", ofilepath);
			return false;
		}

		#ifndef OUTPUT_TO_TXT_FILE
		Write(ofile, "#include \"stdafx.h\"\n");
		Write(ofile, "#include \"Display/Device/D3D9/PCD3D9CapsChecker.h\"\n");
		Write(ofile, "\n");
		Write(ofile, "#ifdef CFG_ALLOW_OPTIMIZE_OFF\n");
		Write(ofile, "//#pragma optimize( "", off ) // CFG_ALLOW_OPTIMIZE_OFF\n");
		Write(ofile, "#endif\n");
		Write(ofile, "\n");
		Write(ofile, "namespace Display\n");
		Write(ofile, "{\n");
		Write(ofile, "namespace D3D9\n");
		Write(ofile, "{\n");
		Write(ofile, "\n");
		Write(ofile, "bool D3DCheck%sCaps(const D3DCAPS9& d3dCaps)\n", ofilename);
		Write(ofile, "{\n");
		Write(ofile, "	bool supported = true;\n");
		Write(ofile, "\n");
		#endif

		C_CapsType& capsType = caps.m_TypeList.back();

		int offset = 0;
		OutputType(ofile, capsType, caps, d3dCaps, offset, "");

		#ifndef OUTPUT_TO_TXT_FILE
		Write(ofile, "\n");
		Write(ofile, "	return supported;\n");
		Write(ofile, "}\n");
		Write(ofile, "\n");
		Write(ofile, "}; // namespace D3D9\n");
		Write(ofile, "}; // namespace Display\n");
		#endif

		ofile.close();
		ofile.clear();
	}

	return true;
}

}; // namespace D3D9
}; // namespace Display
