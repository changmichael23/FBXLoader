
//---------------------------------------- GAUBERT Vincent 4 3DJV 2017 ---------------------------------------

#pragma once

#ifndef ESGI_MAT4_H
#define ESGI_MAT4_H

#include <cstring>
#define _USE_MATH_DEFINES
#include <cmath>
#define M_PI 3.1415926535897
#include <iostream>

namespace Esgi
{
	struct Mat4
	{
		float m[16];

		//Mat4(float diagonal);

		void Identity();
		void MakeScale(float sx, float sy, float sz);
		void SetLocalScale(float sx, float sy, float sz);
		void MakeRotation2D(float angleDegree);
		void MakeRotation3DY(float angleDegree);
		void MakeRotation3DX(float angleDegree);
		void MakeRotation3D(float angleX, float angleY);
		void MakeTranslation(float tx, float ty, float tz);
		void SetTranslate(float tx, float ty, float tz);
		void Perspective(float fovy, float aspect, float nearZ, float farZ);
	};

	void Mat4::Identity()
	{
		memset(m, 0, sizeof(float) * 16);
		m[0] = 1.0f; m[5] = 1.0f; m[10] = 1.0f; m[15] = 1.0f;
		// equivalent a MakeScale(1.0f, 1.0f, 1.0f);
	}

	void Mat4::MakeScale(float sx, float sy, float sz)
	{
		memset(m, 0, sizeof(float) * 16);
		m[0] = sx; m[5] = sy; m[10] = sz; m[15] = 1.0f;
	}

	void Mat4::SetLocalScale(float sx, float sy, float sz)
	{
		m[0] *= sx; m[5] *= sy; m[10] *= sz;
	}

	void Mat4::MakeRotation2D(float angleDegree)
	{
		float theta = angleDegree * (M_PI / 180.0f);
		Identity();
		m[0] = cos(theta);
		m[1] = sin(theta);
		m[4] = -sin(theta);
		m[5] = cos(theta);
	}

	void Mat4::MakeRotation3DY(float angleDegree)
	{
		float theta = angleDegree * (M_PI / 180.0f);
		Identity();
		m[0] = cos(theta);
		m[2] = -sin(theta);
		m[8] = sin(theta);
		m[10] = cos(theta);
	}

	void Mat4::MakeRotation3DX(float angleDegree)
	{
		float theta = angleDegree * (M_PI / 180.0f);
		Identity();
		m[5] = cos(theta);
		m[6] = sin(theta);
		m[9] = -sin(theta);
		m[10] = cos(theta);
	}

	void Mat4::MakeRotation3D(float angleX, float angleY)
	{
		float thetaX = angleX* (M_PI / 180.0f);
		float thetaY = angleY * (M_PI / 180.0f);

		Identity();
		m[0] = cos(thetaY);
		m[2] = -sin(thetaY);
		m[5] = cos(thetaX);
		m[6] = sin(thetaX);
		m[8] = sin(thetaY);
		m[9] = -sin(thetaX);
		m[10] = cos(thetaY) * cos(thetaX);
	}

	void Mat4::MakeTranslation(float tx, float ty, float tz)
	{
		Identity();
		SetTranslate(tx, ty, tz);
	}

	void Mat4::SetTranslate(float tx, float ty, float tz)
	{
		m[12] = tx; m[13] = ty; m[14] = tz;
	}

	void Mat4::Perspective(float fovy, float aspectRatio, float nearZ, float farZ)
	{
		// Projection Matrix
		float fovy_rad = fovy * (M_PI / 180.f);
		float d = 1.0f / tan(fovy_rad * 0.5f);	// Distance focale

		float range = 1.0f / (nearZ - farZ);

		Identity();
		m[0] = d / aspectRatio;
		m[5] = d;
		m[10] = (nearZ + farZ)*range;
		m[11] = -1.0f;
		m[14] = (2.0f * nearZ * farZ)*range;
		m[15] = 0.0f;
	}

}; // Esgi

Esgi::Mat4 operator*(const Esgi::Mat4& A, Esgi::Mat4& B)
{
	// Mij = Sum(k) Aik * Bkj
	Esgi::Mat4 M;
	memset(M.m, 0, sizeof(float) * 16);
	// les boucles j et i sont inversees car on itere
	// colonne apres colonne, mais en memoire les donnees
	// sont justement representees en colonne d'abord 
	// (column major) cad colonne apres colonne
	for (int j = 0; j < 4; j++) 
	{
		for (int i = 0; i < 4; i++) 
		{
			for (int k = 0; k < 4; k++) 
			{
				M.m[i + j * 4] += A.m[i + k * 4] * B.m[k + j * 4];
			}
		}
	}
	return M;
}

Esgi::Mat4 FPSCamera(float tx, float ty, float tz, float rx, float ry)
{ //tx, ty et tz sont les coordonnées de la position de la caméra FPS
	Esgi::Mat4 rotX;
	Esgi::Mat4 rotY;
	Esgi::Mat4 tran;
	Esgi::Mat4 FPS;
	rotX.Identity();
	rotY.Identity();

	rotX.MakeRotation3DY(ry);
	rotY.MakeRotation3DX(rx);

	tran.Identity();
	tran.MakeTranslation(tx, ty, tz);

	FPS.Identity();

	FPS = rotY * rotX * tran;

	return FPS;
}

Esgi::Mat4 OrbitCamera(float px, float py, float pz, float distance, float rx, float ry)
{	//px, py et pz sont les coordonnées x Y et Z du centre de la vue orbit
	//distance est la distance avec le centre
	Esgi::Mat4 rotX;
	Esgi::Mat4 rotY;
	Esgi::Mat4 tran;
	Esgi::Mat4 offset;
	Esgi::Mat4 Orbit;
	rotX.Identity();
	rotY.Identity();

	offset.Identity();
	offset.MakeTranslation(px, py, pz);

	tran.Identity();
	tran.MakeTranslation(0, 0, distance);

	rotX.MakeRotation3DY(ry);
	rotY.MakeRotation3DX(rx);

	Orbit.Identity();

	Orbit = tran * rotY * rotX * offset;

	return Orbit;
}

#endif // ESGI_MAT4_H