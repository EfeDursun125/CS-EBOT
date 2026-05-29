#ifndef STUDIOHDR_H
#define STUDIOHDR_H

// Valve Studio Model Header Structures
// These must match the engine's internal structures

#define STUDIO_VERSION 48

struct mstudiobone_t
{
	int nameindex;
	int parent;
	int bonecontroller[6];
	float value[6];
	float scale[6];
};

struct mstudiohitbox_t
{
	int bone;
	int group;
	Vector bbmin;
	Vector bbmax;
};

struct studiohdr_t
{
	int id;					 // Model format ID (must be "IDST")
	int version;				// Model version
	int checksum;			   // Model checksum for validation
	char name[64];			  // Model name
	int length;				 // Data length
	Vector eyeposition;		 // Eyes position
	Vector min;				 // Bounding box min
	Vector max;				 // Bounding box max
	Vector bbmin;			   // Rendering bounding box min
	Vector bbmax;			   // Rendering bounding box max
	int flags;
	int numbones;			   // Number of bones
	int boneindex;			  // Bones data offset
	int numbonecontrollers;	 // Number of bone controllers
	int bonecontrollerindex;	// Bone controllers offset
	int numhitboxes;			// Number of hitboxes
	int hitboxindex;			// Hitboxes offset
	int numseq;				 // Number of sequences
	int seqindex;			   // Sequences offset
	int numseqgroups;		   // Number of sequence groups
	int seqgroupindex;		  // Sequence groups offset
	int numtransitions;		 // Animation transitions
	int transitionindex;		// Transitions offset
};

inline mstudiobone_t* getBone(studiohdr_t* studio, int i)
{
	return reinterpret_cast<mstudiobone_t*>(reinterpret_cast<uint8_t*>(studio) + studio->boneindex + i * sizeof(mstudiobone_t));
}

inline mstudiohitbox_t* getHitbox(studiohdr_t* studio, int i)
{
	return reinterpret_cast<mstudiohitbox_t*>(reinterpret_cast<uint8_t*>(studio) + studio->hitboxindex + i * sizeof(mstudiohitbox_t));
}

inline int getHitboxByName(studiohdr_t* studio, const char* boneName)
{
	for (int i = 0; i < studio->numhitboxes; i++)
	{
		mstudiohitbox_t* hitbox = getHitbox(studio, i);
		mstudiobone_t* bone = getBone(studio, hitbox->bone);
		const char* name = reinterpret_cast<const char*>(reinterpret_cast<uint8_t*>(studio) + bone->nameindex);
		if (!cstrcmp(name, boneName))
			return i;
	}
	return -1;
}

#endif