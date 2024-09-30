#pragma once
#include "../rendering/Material.h"
#include "Sector.h"

class Level
{
public:
	void Construct()
	{

	}

private:
	std::vector<Sector> m_sectors;
	std::vector<Material> m_materials;
};

