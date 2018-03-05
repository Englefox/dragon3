#include <string>

#define NUM_BONES_PER_VERTEX 4
struct VertexBoneData
{
	unsigned int IDs[NUM_BONES_PER_VERTEX];
	float Weights[NUM_BONES_PER_VERTEX];

	VertexBoneData()
	{
		Reset();
	}

	void Reset()
	{
		memset(&IDs, 0, sizeof(IDs));
		memset(&Weights, 0, sizeof(Weights));
	}

	void AddBoneData(unsigned int BoneID, float Weight)
	{
		for (unsigned int i = 0; i < 4; i++)
		{
			int n = Weights[0];
			if (Weights[i] == 0.0) {
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			}
		}
		//assert(0);
	}
};
