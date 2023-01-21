// A XML parser to read and setup a level - uses TinyXML2 to parse the file into its own structure, the 
// methods in this class traverse that structure and create entities and templates as appropriate

#include "BaseMath.h"
#include "Entity.h"
#include "TankEntity.h"
#include "ParseLevel.h"

// This kind of statement would be bad practice in a include file, but here in a cpp it is a reasonable convenience
// since this file is dedicated to this namespace (and the exposed names won't leak into other parts of the program)
using namespace tinyxml2;


namespace gen
{

	// Parse the entire level file and create all the templates and entities inside
	bool CParseLevel::ParseFile(const string& fileName)
	{
		// The tinyXML object XMLDocument will hold the parsed structure and data from the XML file
		// NOTE: even though there is a "using namespace tinyxml2;" at the top of the file, you still need to
		// give this declaration a tinyxml2:: because the Windows header files have also declared XMLDocument
		// which is not in a namespace so it collides with the TinyXML2 definition. An excellent example about
		// why namespaces are important (and this occurs more than once in Windows header files)
		tinyxml2::XMLDocument xmlDoc;

		// Open and parse document into tinyxml2 object xmlDoc
		XMLError error = xmlDoc.LoadFile(fileName.c_str());
		if (error != XML_SUCCESS)  return false;

		// Note: I am ignoring XML nodes here and traversing the XML elements directly. Elements are a kind of node, they are
		// the tags but other kinds of node are comments, embedded documents or free text (not in a tag). I am ignoring those

		// No XML element in the level file means malformed XML or not an XML document at all
		XMLElement* element = xmlDoc.FirstChildElement();
		if (element == nullptr)  return false;

		while (element != nullptr)
		{
			// We found a "Level" tag at the root level, parse it
			string elementName = element->Name();
			if (elementName == "Level")
			{
				ParseLevelElement(element);
			}

			//else if (elementName == "xxx")
			//{
			//   You would add code for other document root level elements here, but our file only has "Level"
			//}

			element = element->NextSiblingElement();
		}

		return true;
	}



	// Parse a "Level" tag within the level XML file
	bool CParseLevel::ParseLevelElement(XMLElement* rootElement)
	{
		XMLElement* element = rootElement->FirstChildElement();
		while (element != nullptr)
		{
			// Things expected in a "Level" tag
			string elementName = element->Name();
			if (elementName == "Templates")  ParseTemplatesElement(element);
			else if (elementName == "Entities")  ParseEntitiesElement(element);
			// You could add more tags within Level here (not needed for exercise, just saying)

			element = element->NextSiblingElement();
		}

		return true;
	}



	// Parse a list of entity template tags, will create each template when enough data has been read
	bool CParseLevel::ParseTemplatesElement(XMLElement* rootElement)
	{
		XMLElement* element = rootElement->FirstChildElement("EntityTemplate");
		while (element != nullptr)
		{
			// Read the type, name and mesh attributes - these are all required, so fail on error
			const XMLAttribute* attr = element->FindAttribute("Type");
			if (attr == nullptr)  return false;
			string type = attr->Value();

			attr = element->FindAttribute("Name");
			if (attr == nullptr)  return false;
			string name = attr->Value();

			attr = element->FindAttribute("Mesh");
			if (attr == nullptr)  return false;
			string mesh = attr->Value();


			// We can create the entity template now for most types
			
			if (type == "Tank")
			{
				// Previous attributes all returned strings, these attributes contain ints and floats
				attr = element->FindAttribute("MaxSpeed");
				if (attr == nullptr)  return false;
				float maxSpeed = attr->FloatValue();

				attr = element->FindAttribute("Acceleration");
				if (attr == nullptr)  return false;
				float acceleration = attr->FloatValue();

				attr = element->FindAttribute("TurnSpeed");
				if (attr == nullptr)  return false;
				float turnSpeed = attr->FloatValue();

				attr = element->FindAttribute("TurretTurnSpeed");
				if (attr == nullptr)  return false;
				float turretTurnSpeed = attr->FloatValue();

				attr = element->FindAttribute("MaxHP");
				if (attr == nullptr)  return false;
				int maxHP = attr->IntValue();

				attr = element->FindAttribute("ShellDamage");
				if (attr == nullptr)  return false;
				int shellDamage = attr->IntValue();

				m_EntityManager->CreateTankTemplate(type, name, mesh, maxSpeed, acceleration,
					turnSpeed, turretTurnSpeed, maxHP, shellDamage);
			}
			// Tank types have additional required attributes, get them now
			else
			{
				m_EntityManager->CreateTemplate(type, name, mesh);
			}

			// Find next entity template
			element = element->NextSiblingElement("EntityTemplate");
		}

		return true;
	}


	// Parse a list of entity tags, will create each entity when enough data has been read
	// Some entities are collected into teams. The enitites in teams are parsed in a seperate pass after ordinary entities
	bool CParseLevel::ParseEntitiesElement(XMLElement* rootElement)
	{
		//--------------------
		// Ordinary entities

		// First parse the ordinary entities (things not in a <Team> tag)

		XMLElement* element = rootElement->FirstChildElement("Entity");
		int entityCount;
		while (element != nullptr)
		{
			entityCount = 1;

			// Read the type and name attributes - these are required, so fail on error
			const XMLAttribute* attr = element->FindAttribute("Type");
			if (attr == nullptr)  return false;
			string type = attr->Value();

			attr = element->FindAttribute("Name");
			if (attr == nullptr)  return false;
			string name = attr->Value();

			attr = element->FindAttribute("Number");
			entityCount = (attr == nullptr) ? 1 : attr->IntValue();


			for (int i = 0; i < entityCount; i++)
			{
				// Next check for optional child elements such as position or scale, but set default values for all in case they are not provided
				CVector3 pos{ 0, 0, 0 };
				CVector3 rot{ 0, 0, 0 };
				CVector3 scale{ 1, 1, 1 };

				XMLElement* child = element->FirstChildElement("Position");
				if (child != nullptr)  pos = GetVector3FromElement(child); // Helper method will read the X,Y,Z attributes into a CVector3

				child = element->FirstChildElement("Rotation");
				if (child != nullptr)
				{
					rot = GetVector3FromElement(child);
					rot.x = ToRadians(rot.x);
					rot.y = ToRadians(rot.y);
					rot.z = ToRadians(rot.z);
				}

				child = element->FirstChildElement("Scale");
				if (child != nullptr)  scale = GetVector3FromElement(child);

				string templateType = m_EntityManager->GetTemplate(type)->GetType();
				if (templateType != "Tank")
				{
					if (templateType == "Ammo" || templateType == "Health")
					{
						attr = element->FindAttribute("RotationSpeed");
						if (attr == nullptr)  return false;
						float rotationSpeed = attr->FloatValue();

						attr = element->FindAttribute("RespawnTime");
						if (attr == nullptr)  return false;
						float respawnTime = Random(5.0f, attr->FloatValue());

						attr = element->FindAttribute("PickUpDistance");
						if (attr == nullptr)  return false;
						float pickUpDistance = attr->FloatValue();

						m_EntityManager->CreateCrate(type, rotationSpeed, respawnTime,
							pickUpDistance, name, pos, rot, scale);	
					}
					else if (templateType == "Mine")
					{
						attr = element->FindAttribute("RespawnTime");
						if (attr == nullptr)  return false;
						float respawnTime = attr->FloatValue();

						attr = element->FindAttribute("DamageRadius");
						if (attr == nullptr)  return false;
						float damageRadius = Random(5.0f, attr->FloatValue());

						m_EntityManager->CreateMine(type, respawnTime, damageRadius, 
							name, pos, rot, scale);
					}
					else if (templateType == "Projectile")
					{
						attr = element->FindAttribute("OwnerName");
						if (attr == nullptr)  return false;
						string ownerName= attr->Value();

						CEntity* owner = m_EntityManager->GetEntity(ownerName);
						if (owner != 0)
						{
							CTankEntity* ownerTank = static_cast<CTankEntity*>(owner);
							m_EntityManager->CreateShell(type, ownerTank, name, pos, rot, scale);
						}
					}
					else
					{
						m_EntityManager->CreateEntity(type, name, pos, rot, scale);
					}
				}
				else
				{
					attr = element->FindAttribute("Team");
					if (attr == nullptr)  return false;
					int team = attr->IntValue();

					// Waypoints
					child = element->FirstChildElement("ListWaypoints");
					vector<CVector3> patrolPoints;
					if (child != nullptr)
					{
						XMLElement* waypointElement = child->FirstChildElement("Waypoint");
						while (waypointElement != nullptr)
						{
							CVector3 waypoint = GetVector3FromElement(waypointElement);
							patrolPoints.push_back(waypoint);
							waypointElement = waypointElement->NextSiblingElement("Waypoint");
						}
					}

					m_EntityManager->CreateTank(type, team, patrolPoints, name, pos, rot, scale);
				}
			}
				
			// Next ordinary entity
			element = element->NextSiblingElement("Entity");
		}

		return true;
	}


	// Helper method to read a CVector3 from an element, expecting X, Y and Z attributes.
	// Also supports a "Randomise" feature, see code
	CVector3 CParseLevel::GetVector3FromElement(XMLElement* element)
	{
		CVector3 vector{ 0, 0, 0 };

		const XMLAttribute* attr = element->FindAttribute("X");
		if (attr != nullptr)  vector.x = attr->FloatValue();

		attr = element->FindAttribute("Y");
		if (attr != nullptr)  vector.y = attr->FloatValue();

		attr = element->FindAttribute("Z");
		if (attr != nullptr)  vector.z = attr->FloatValue();

		// We also support a "Randomise" tag within any CVector3 type tag, it's another vector3 that randomises the first
		XMLElement* child = element->FirstChildElement("Randomise");
		if (child != nullptr)
		{
			float random = 0;

			attr = child->FindAttribute("X");
			if (attr != nullptr)  random = attr->FloatValue() * 0.5f;
			vector.x += Random(-random, random);

			attr = child->FindAttribute("Y");
			if (attr != nullptr)  random = attr->FloatValue() * 0.5f;
			vector.y += Random(-random, random);

			attr = child->FindAttribute("Z");
			if (attr != nullptr)  random = attr->FloatValue() * 0.5f;
			vector.z += Random(-random, random);
		}

		return vector;
	}


} // namespace gen
