#ifndef RENDER_DYNAMICEFFECT_PARTICLE_PHYSICSSETTINGS_H
#define RENDER_DYNAMICEFFECT_PARTICLE_PHYSICSSETTINGS_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Dynamic/Particle/Enums.h"

namespace DynamicEffect
{
namespace Particle
{

struct PhysicsSettings					// shared settings between all types of particle systems
{
	PhysicsSettings() : eBehaviour(E_PhysicsBehaviour_Default), eCollision(E_Collision_None), fRestitution(0.f), fFriction(0.f), fAirResistance(.0f), vertexSize(0), bDieOnImpact(false) {}
	~PhysicsSettings() {}
	
	E_PhysicsBehaviour		eBehaviour;
	E_Collision				eCollision;
	
	F32			fRestitution;				// collision response
	F32			fFriction;					// collision response
	F32			fAirResistance;

	U32			vertexSize;
	bool		bDieOnImpact;

	MathsStorage::Float4A16		vAcceleration;
};

}
}

#endif // RENDER_DYNAMICEFFECT_PARTICLE_PHYSICSSETTINGS_H
