#pragma once

#include <shared_mutex>
#include "WorldPosition.h"

//THEORY
// 
// Pathfinding in (c)mangos is based on detour recast an opensource nashmesh creation and pathfinding codebase.
// This system is used for mob and npc pathfinding and in this codebase also for bots.
// Because mobs and npc movement is based on following a player or a set path the pathfinder is limited to 296y.
// This means that when trying to find a path from A to B distances beyond 296y will be a best guess often moving in a straight path.
// Bots would get stuck moving from Northshire to Stormwind because there is no 296y path that doesn't go (initially) the wrong direction.
// 
// To remedy this limitation without altering the pathfinder limits too much this node system was introduced.
// 
//  <S> ---> [N1] ---> [N2] ---> [N3] ---> <E>
//
// Bot at <S> wants to move to <E>
// [N1],[N2],[N3] are predefined nodes for wich we know we can move from [N1] to [N2] and from [N2] to [N3] but not from [N1] to [N3]
// If we can move fom [S] to [N1] and from [N3] to [E] we have a complete route to travel.
// 
// Termonology:
// Node: a location on a map for which we know bots are likely to want to travel to or need to travel past to reach other nodes.
// Link: the connection between two nodes. A link signifies that the bot can travel from one node to another. A link is one-directional.
// Path: the waypointpath returned by the standard pathfinder to move from one node (or position) to another. A path can be imcomplete or empty which means there is no link.
// Route: the list of nodes that give the shortest route from a node to a distant node. Routes are calculated using a standard A* search based on links.
// 
// On server start saved nodes and links are loaded. Paths and routes are calculated on the fly but saved for future use.
// Nodes can be added and removed realtime however because bots access the nodes from different threads this requires a locking mechanism.
// 
// Initially the current nodes have been made:
// Flightmasters and Inns (Bots can use these to fast-travel so eventually they will be included in the route calculation)
// WorldBosses and Unique bosses in instances (These are a logical places bots might want to go in instances)
// Player start spawns (Obviously all lvl1 bots will spawn and move from here)
// Area triggers locations with teleport and their teleport destinations (These used to travel in or between maps)
// Transports including elevators (Again used to travel in and in maps)
// (sub)Zone means (These are the center most point for each sub-zone which is good for global coverage)
// 
// To increase coverage/linking extra nodes can be automatically be created.
// Current implentation places nodes on paths (including complete) at sub-zone transitions or randomly.
// After calculating possible links the node is removed if it does not create local coverage.
//

    
enum class TravelNodePathType : uint8
{
    none = 0,
    walk = 1,
    areaTrigger = 2,
    transport = 3,
    flightPath = 4,
    teleportSpell = 5,
    staticPortal = 6
};

using namespace ai;

namespace ai
{
    class WorldPosition;

    //A connection between two nodes. 
    class TravelNodePath
    {
    public:
        //Legacy Constructor for travelnodestore
        //TravelNodePath(float distance1, float extraCost1, bool portal1 = false, uint32 portalId1 = 0, bool transport1 = false, bool calculated = false, uint8 maxLevelMob1 = 0, uint8 maxLevelAlliance1 = 0, uint8 maxLevelHorde1 = 0, float swimDistance1 = 0, bool flightPath1 = false);

        //Constructor
        TravelNodePath(float distance = 0.1f, float extraCost = 0, uint8 pathType = (uint8)TravelNodePathType::walk, uint64 pathObject = 0, bool calculated = false, vector<uint8> maxLevelCreature = { 0,0,0 }, float swimDistance = 0)
            : distance(distance), extraCost(extraCost), pathType(TravelNodePathType(pathType)), pathObject(pathObject), calculated(calculated), maxLevelCreature(maxLevelCreature), swimDistance(swimDistance) {
            if (pathType != (uint8)TravelNodePathType::walk) complete = true;
        };

        TravelNodePath(TravelNodePath* basePath) {
            complete = basePath->complete; path = basePath->path; extraCost = basePath->extraCost; calculated = basePath->calculated; distance = basePath->distance; maxLevelCreature = basePath->maxLevelCreature; swimDistance = basePath->swimDistance; pathType = basePath->pathType; pathObject = basePath->pathObject;
        };

        //Getters
        bool getComplete() { return complete || pathType != TravelNodePathType::walk; }
        vector<WorldPosition> getPath() { return path; }

        TravelNodePathType getPathType() { return pathType; }
        uint64 getPathObject() { return pathObject; }

        float getDistance() { return distance; }
        float getSwimDistance() { return swimDistance; }
        float getExtraCost() { return extraCost; }
        vector<uint8> getMaxLevelCreature() { return  maxLevelCreature; }

        void setCalculated(bool calculated1 = true) { calculated = calculated1; }
        bool getCalculated() { return calculated; }

        string print();

        //Setters
        void setComplete(bool complete1) { complete = complete1; }
        void setPath(vector<WorldPosition> path1) { path = path1; }
        void setPathAndCost(vector<WorldPosition> path1, float speed) { setPath(path1); calculateCost(true); extraCost = distance / speed; }
        //void setPortal(bool portal1, uint32 portalId1 = 0) { portal = portal1; portalId = portalId1; }
        //void setTransport(bool transport1) { transport = transport1; }
        void setPathType(TravelNodePathType pathType1) { pathType = pathType1; }
        void setPathObject(uint64 pathObject1) { pathObject = pathObject1; }

        void calculateCost(bool distanceOnly = false);

        float getCost(Player* bot = nullptr, uint32 cGold = 0);
        uint32 getPrice();
    private:
        //Does the path have all the points to get to the destination?
        bool complete = false;

        //List of WorldPositions to get to the destination.
        vector<WorldPosition> path = {};

        //The extra (loading/transport) time it takes to take this path.
        float extraCost = 0;

        bool calculated = false;

        //Derived distance in yards
        float distance = 0.1f;

        //Calculated mobs level along the way.
        vector<uint8> maxLevelCreature = { 0,0,0 }; //mobs, horde, alliance

        //Calculated swiming distances along the way.
        float swimDistance = 0;

        TravelNodePathType pathType = TravelNodePathType::walk;
        uint64 pathObject = 0;

        /*
        //Is the path a portal/teleport to the destination?
        bool portal = false;
        //Area trigger Id
        uint32 portalId = 0;

        //Is the path transport based?
        bool transport = false;

        //Is the path a flightpath?
        bool flightPath = false;
        */
    };

    //A waypoint to travel from or to.
    //Each node knows which other nodes can be reached without help.
    class TravelNode
    {
    public:
        //Constructors
        TravelNode() {};
        TravelNode(WorldPosition point1, string nodeName1 = "Travel Node", bool important1 = false) { nodeName = nodeName1; point = point1; important = important1; }
        TravelNode(TravelNode* baseNode) { nodeName = baseNode->nodeName; point = baseNode->point; important = baseNode->important; }

        //Setters
        void setLinked(bool linked1) { linked = linked1; }
        void setPoint(WorldPosition point1) { point = point1; }

        //Getters
        string getName() { return nodeName; };
        WorldPosition* getPosition() { return &point; };
        std::unordered_map<TravelNode*, TravelNodePath>* getPaths() { return &paths; }
        std::unordered_map<TravelNode*, TravelNodePath*>* getLinks() { return &links; }
        bool isImportant() { return important; };
        bool isLinked() { return linked; }
        bool isTransport() { for (auto link : *getLinks()) if (link.second->getPathType() == TravelNodePathType::transport) return true; return false; }
        uint32 getTransportId() { for (auto link : *getLinks()) if (link.second->getPathType() == TravelNodePathType::transport) return link.second->getPathObject(); return false; }
        bool isPortal() { for (auto link : *getLinks()) if (link.second->getPathType() == TravelNodePathType::areaTrigger || link.second->getPathType() == TravelNodePathType::staticPortal) return true; return false; }

        //WorldLocation shortcuts
        uint32 getMapId() { return point.getMapId(); }
        float getX() { return point.getX(); }
        float getY() { return point.getY(); }
        float getZ() { return point.getZ(); }
        float getO() { return point.getO(); }
        float getDistance(WorldPosition pos) { return point.distance(pos); }
        float getDistance(TravelNode* node) { return point.distance(*node->getPosition()); }
        float fDist(WorldPosition pos) { return point.fDist(pos); }
        float fDist(TravelNode* node) { return point.fDist(*node->getPosition()); }

        TravelNodePath* setPathTo(TravelNode* node, TravelNodePath path = TravelNodePath(), bool isLink = true) { if (this != node) { paths[node] = path; if (isLink) links[node] = &paths[node]; return  &paths[node]; } else return nullptr; }
        bool hasPathTo(TravelNode* node) { return paths.find(node) != paths.end(); }
        TravelNodePath* getPathTo(TravelNode* node) { return &paths[node]; }
        bool hasCompletePathTo(TravelNode* node) { return hasPathTo(node) && getPathTo(node)->getComplete(); }
        TravelNodePath* buildPath(TravelNode* endNode, Unit* bot, bool postProcess = false);

        void setLinkTo(TravelNode* node, float distance = 0.1f) {
            if (this != node)
            {
                if (!hasPathTo(node))
                    setPathTo(node, TravelNodePath(distance));
                else
                    links[node] = &paths[node];
            }
        }

        bool hasLinkTo(TravelNode* node) { return links.find(node) != links.end(); }
        float linkCostTo(TravelNode* node) { return paths.find(node)->second.getDistance(); }
        float linkDistanceTo(TravelNode* node) { return paths.find(node)->second.getDistance(); }
        void removeLinkTo(TravelNode* node, bool removePaths = false);

        bool isEqual(TravelNode* compareNode);

        //Removes links to other nodes that can also be reached by passing another node.
        bool isUselessLink(TravelNode* farNode);
        void cropUselessLink(TravelNode* farNode);
        bool cropUselessLinks();

        //Returns all nodes that can be reached from this node.
        vector<TravelNode*> getNodeMap(bool importantOnly = false, vector<TravelNode*> ignoreNodes = {}, bool mapOnly = false);

        //Checks if it is even possible to route to this node.
        bool hasRouteTo(TravelNode* node, bool mapOnly = false) { if (routes.empty()) for (auto mNode : getNodeMap(mapOnly)) routes[mNode] = true; return routes.find(node) != routes.end(); };
        void clearRoutes() { routes.clear(); }

        void print(bool printFailed = true);
    protected:
        //Logical name of the node
        string nodeName;
        //WorldPosition of the node.
        WorldPosition point;

        //List of paths to other nodes.
        std::unordered_map<TravelNode*, TravelNodePath> paths;
        //List of links to other nodes.
        std::unordered_map<TravelNode*, TravelNodePath*> links;

        //List of nodes and if there is 'any' route possible
        std::unordered_map<TravelNode*, bool> routes;

        //This node should not be removed
        bool important = false;

        //This node has been checked for nearby links
        bool linked = false;

        //This node is a (moving) transport.
        //bool transport = false;
        //Entry of transport.
        //uint32 transportId = 0;
    };

    class PortalNode : public TravelNode
    {
    public:
        PortalNode(TravelNode* baseNode) : TravelNode(baseNode) {};

        void SetPortal(TravelNode* baseNode, TravelNode* endNode, uint32 portalSpell)
        {
            nodeName = baseNode->getName();
            point = *baseNode->getPosition();
            paths.clear();
            links.clear();
            TravelNodePath path(0.1f, 0.1f, (uint8)TravelNodePathType::teleportSpell, portalSpell, true);
            setPathTo(endNode, path);
        };
    };

    //Route step type
    enum class PathNodeType : uint8
    {
        NODE_PREPATH = 0,
        NODE_PATH = 1,
        NODE_NODE = 2,
        NODE_AREA_TRIGGER = 3,
        NODE_TRANSPORT = 4,
        NODE_FLIGHTPATH = 5,
        NODE_TELEPORT = 6,
        NODE_STATIC_PORTAL = 7
    };

    struct PathNodePoint
    {
        WorldPosition point;
        PathNodeType type = PathNodeType::NODE_PATH;
        uint32 entry = 0;
    };

    //A complete list of points the bots has to walk to or teleport to.
    class TravelPath
    {
    public:
        TravelPath() {};
        TravelPath(vector<PathNodePoint> fullPath1) { fullPath = fullPath1; }
        TravelPath(vector<WorldPosition> path, PathNodeType type = PathNodeType::NODE_PATH, uint32 entry = 0) { addPath(path, type, entry); }

        void addPoint(PathNodePoint point) { fullPath.push_back(point); }
        void addPoint(WorldPosition point, PathNodeType type = PathNodeType::NODE_PATH, uint32 entry = 0) { fullPath.push_back(PathNodePoint{ point, type, entry }); }
        void addPath(vector<WorldPosition> path, PathNodeType type = PathNodeType::NODE_PATH, uint32 entry = 0) { for (auto& p : path) { fullPath.push_back(PathNodePoint{ p, type, entry }); }; }
        void addPath(vector<PathNodePoint> newPath) { fullPath.insert(fullPath.end(), newPath.begin(), newPath.end()); }
        void clear() { fullPath.clear(); }

        bool empty() { return fullPath.empty(); }
        vector<PathNodePoint> getPath() { return fullPath; }
        WorldPosition getFront() { return fullPath.front().point; }
        WorldPosition getBack() { return fullPath.back().point; }

        vector<WorldPosition> getPointPath() { vector<WorldPosition> retVec; for (const auto& p : fullPath) retVec.push_back(p.point); return retVec; };

        bool makeShortCut(WorldPosition startPos, float maxDist, Unit* bot);
        bool shouldMoveToNextPoint(WorldPosition startPos, vector<PathNodePoint>::iterator beg, vector<PathNodePoint>::iterator ed, vector<PathNodePoint>::iterator p, float& moveDist, float maxDist);
        WorldPosition getNextPoint(WorldPosition startPos, float maxDist, TravelNodePathType& pathType, uint32& entry);

        ostringstream print();
    private:
        vector<PathNodePoint> fullPath;
    };

    //An stored A* search that gives a complete route from one node to another.
    class TravelNodeRoute
    {
    public:
        TravelNodeRoute() {}
        TravelNodeRoute(vector<TravelNode*> nodes1, TravelNode* tempNode = nullptr) { nodes = nodes1; if (tempNode) addTempNode(tempNode); }

        bool isEmpty() { return nodes.empty(); }

        bool hasNode(TravelNode* node) { return findNode(node) != nodes.end(); }
        float getTotalDistance();

        void setNodes(vector<TravelNode*> nodes1) { nodes = nodes1; }
        vector<TravelNode*> getNodes() { return nodes; }

        void addTempNode(TravelNode* node) { tempNodes.push_back(node); }
        void cleanTempNodes() { for (auto node : tempNodes) delete node; }

        TravelPath buildPath(vector<WorldPosition> pathToStart = {}, vector<WorldPosition> pathToEnd = {}, Unit* bot = nullptr);

        ostringstream print();
    private:
        vector<TravelNode*>::iterator findNode(TravelNode* node) { return std::find(nodes.begin(), nodes.end(), node); }
        vector<TravelNode*> nodes;
        vector<TravelNode*> tempNodes;
    };

    //A node container to aid A* calculations with nodes.
    class TravelNodeStub
    {
    public:
        TravelNodeStub(TravelNode* dataNode1) { dataNode = dataNode1; }

        TravelNode* dataNode;
        float m_f = 0.0, m_g = 0.0, m_h = 0.0;
        bool open = false, close = false;
        TravelNodeStub* parent = nullptr;
        uint32 currentGold = 0.0;
    };

    //The container of all nodes.
    class TravelNodeMap
    {
    public:
        TravelNodeMap() {};
        TravelNodeMap(TravelNodeMap* baseMap);

        TravelNode* addNode(WorldPosition pos, string preferedName = "Travel Node", bool isImportant = false, bool checkDuplicate = true, bool transport = false, uint32 transportId = 0);
        void removeNode(TravelNode* node);
        bool removeNodes() { if (m_nMapMtx.try_lock_for(std::chrono::seconds(10))) { for (auto& node : m_nodes) removeNode(node); m_nMapMtx.unlock(); return true; } return false; };
        void fullLinkNode(TravelNode* startNode, Unit* bot);

        //Get all nodes
        vector<TravelNode*> getNodes() { return m_nodes; }
        vector<TravelNode*> getNodes(WorldPosition pos, float range = -1);

        //Find nearest node.
        TravelNode* getNode(TravelNode* sameNode) { for (auto& node : m_nodes) { if (node->getName() == sameNode->getName() && node->getPosition() == sameNode->getPosition()) return node; } return nullptr; }
        TravelNode* getNode(WorldPosition pos, vector<WorldPosition>& ppath, Unit* bot = nullptr, float range = -1);
        TravelNode* getNode(WorldPosition pos, Unit* bot = nullptr, float range = -1) { vector<WorldPosition> ppath; return getNode(pos, ppath, bot, range); }

        //Get Random Node
        TravelNode* getRandomNode(WorldPosition pos) { vector<TravelNode*> rNodes = getNodes(pos); if (rNodes.empty()) return nullptr; return  rNodes[urand(0, rNodes.size() - 1)]; }

        //Finds the best nodePath between two nodes
        TravelNodeRoute getRoute(TravelNode* start, TravelNode* goal, Player* bot = nullptr);

        //Find the best node between two positions
        TravelNodeRoute getRoute(WorldPosition startPos, WorldPosition endPos, vector<WorldPosition>& startPath, Player* bot = nullptr);

        //Find the full path between those locations
        static TravelPath getFullPath(WorldPosition startPos, WorldPosition endPos, Player* bot = nullptr);

        //Manage/update nodes
        void manageNodes(Unit* bot, bool mapFull = false);

        void setHasToGen() { hasToGen = true; }
        bool gethasToGen() { return hasToGen || hasToFullGen; }

        void generateNpcNodes();
        void generateStartNodes();
        void generateAreaTriggerNodes();
        void generateNodes();
        void generateTransportNodes();
        void generateZoneMeanNodes();
        void generatePortalNodes();

        void generateWalkPaths();
        void generateWalkPathMap(uint32 mapId);
        void removeLowNodes();
        void removeUselessPaths();
        void removeUselessPathMap(uint32 mapId);
        void calculatePathCosts();
        void generateTaxiPaths();
        void generatePaths();

        void generateAll();

        void printMap();

        void printNodeStore();
        void saveNodeStore();
        void loadNodeStore();

        bool cropUselessNode(TravelNode* startNode);
        TravelNode* addZoneLinkNode(TravelNode* startNode);
        TravelNode* addRandomExtNode(TravelNode* startNode);

        void calcMapOffset();
        WorldPosition getMapOffset(uint32 mapId);

        std::shared_timed_mutex m_nMapMtx;
    private:
        vector<TravelNode*> m_nodes;

        vector<pair<uint32, WorldPosition>> mapOffsets;

        bool hasToSave = false;
        bool hasToGen = false;
        bool hasToFullGen = false;
    };
}

#define sTravelNodeMap MaNGOS::Singleton<TravelNodeMap>::Instance()