/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "assimp_view.h"

using namespace AssimpView;

// ------------------------------------------------------------------------------------------------
// Constructor on a given animation. 
AnimEvaluator::AnimEvaluator( const aiAnimation* pAnim)
{
	mAnim = pAnim;
	mLastTime = 0.0;
	mLastPositions.resize( pAnim->mNumChannels, boost::make_tuple( 0, 0, 0));
}

// ------------------------------------------------------------------------------------------------
// Evaluates the animation tracks for a given time stamp. 
void AnimEvaluator::Evaluate( double pTime)
{
	// extract ticks per second. Assume default value if not given
	double ticksPerSecond = mAnim->mTicksPerSecond != 0.0 ? mAnim->mTicksPerSecond : 25.0;
	// every following time calculation happens in ticks
	pTime *= ticksPerSecond;

	// map into anim's duration
	double time = 0.0f;
	if( mAnim->mDuration > 0.0)
		time = fmod( pTime, mAnim->mDuration);

	if( mTransforms.size() != mAnim->mNumChannels)
		mTransforms.resize( mAnim->mNumChannels);

	// calculate the transformations for each animation channel
	for( unsigned int a = 0; a < mAnim->mNumChannels; a++)
	{
		const aiNodeAnim* channel = mAnim->mChannels[a];

		// ******** Position *****
		aiVector3D presentPosition( 0, 0, 0);
		if( channel->mNumPositionKeys > 0)
		{
			// Look for present frame number. Search from last position if time is after the last time, else from beginning
			// Should be much quicker than always looking from start for the average use case.
			unsigned int frame = (time >= mLastTime) ? mLastPositions[a].get<0>() : 0;
			while( frame < channel->mNumPositionKeys - 1)
			{
				if( time < channel->mPositionKeys[frame+1].mTime)
					break;
				frame++;
			}

			// TODO: (thom) interpolation maybe?
			presentPosition = channel->mPositionKeys[frame].mValue;
			mLastPositions[a].get<0>() = frame;
		}

		// ******** Rotation *********
		aiQuaternion presentRotation( 1, 0, 0, 0);
		if( channel->mNumRotationKeys > 0)
		{
			unsigned int frame = (time >= mLastTime) ? mLastPositions[a].get<1>() : 0;
			while( frame < channel->mNumRotationKeys - 1)
			{
				if( time < channel->mRotationKeys[frame+1].mTime)
					break;
				frame++;
			}

			// TODO: (thom) quaternions are a prime target for interpolation
			presentRotation = channel->mRotationKeys[frame].mValue;
			mLastPositions[a].get<1>() = frame;
		}

		// ******** Scaling **********
		aiVector3D presentScaling( 1, 1, 1);
		if( channel->mNumScalingKeys > 0)
		{
			unsigned int frame = (time >= mLastTime) ? mLastPositions[a].get<2>() : 0;
			while( frame < channel->mNumScalingKeys - 1)
			{
				if( time < channel->mScalingKeys[frame+1].mTime)
					break;
				frame++;
			}

			// TODO: (thom) interpolation maybe? This time maybe even logarithmic, not linear
			presentScaling = channel->mScalingKeys[frame].mValue;
			mLastPositions[a].get<2>() = frame;
		}

		// build a transformation matrix from it
		aiMatrix4x4& mat = mTransforms[a];
		mat = aiMatrix4x4( presentRotation.GetMatrix());
		mat.a1 *= presentScaling.x; mat.b1 *= presentScaling.x; mat.c1 *= presentScaling.x;
		mat.a2 *= presentScaling.y; mat.b2 *= presentScaling.y; mat.c2 *= presentScaling.y;
		mat.a3 *= presentScaling.z; mat.b3 *= presentScaling.z; mat.c3 *= presentScaling.z;
		mat.a4 = presentPosition.x; mat.b4 = presentPosition.y; mat.c4 = presentPosition.z;
		//mat.Transpose();
	}

	mLastTime = time;
}
