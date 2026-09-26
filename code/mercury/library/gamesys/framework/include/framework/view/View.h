#ifndef FRAMEWORK_VIEW_H
#define FRAMEWORK_VIEW_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Render/View/View.h"

namespace Cam2
{
	class C_DebugFeed;
}

namespace Framework
{

//! C_View

class C_View : public Render::C_View
{
public:
	C_View();
	~C_View();

	const Cam2::C_DebugFeed& DebugFeed() const
	{
		assert(m_pDebugFeed);
		return *m_pDebugFeed;
	}

	Cam2::C_DebugFeed& DebugFeed()
	{
		assert(m_pDebugFeed);
		return *m_pDebugFeed;
	}
	
	void Update(const S32 index, const F32 frameTime);

private:
	Cam2::C_DebugFeed* m_pDebugFeed;

};

}; // namespace Framework

#endif // FRAMEWORK_VIEW_H
