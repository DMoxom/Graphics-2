#pragma once
#include "SceneNode.h"
#include <list>

using namespace std;

typedef list<SceneNodePointer> SceneNodeList;
typedef SceneNodeList::iterator SceneGraphIterator;

class SceneGraph : public SceneNode
{
public:
	SceneGraph() : SceneNode(L"Root") {};
	SceneGraph(wstring name) : SceneNode(name) {};
	~SceneGraph(void) {};

	virtual bool Initialise(void);
	virtual void Update(FXMMATRIX& currentWorldTransformation);
	virtual void Render(void);
	virtual void Shutdown(void);

	void Add(SceneNodePointer node);
	void Remove(SceneNodePointer node);
	SceneNodePointer Find(wstring name);

private:
	SceneNodeList _children;
};

typedef shared_ptr<SceneGraph>			 SceneGraphPointer;
