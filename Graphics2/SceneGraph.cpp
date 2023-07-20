#include "SceneGraph.h"

bool SceneGraph::Initialise(void)
{
	// Recursively initialise all child noes
	bool status = true;

	for (SceneGraphIterator listIterator = begin(_children);
		listIterator != end(_children) && status;
		listIterator++)
	{
		status = (*listIterator)->Initialise();
	}
	return status;
}

void SceneGraph::Update(FXMMATRIX& currentWorldTransformation)
{
	SceneNode::Update(currentWorldTransformation);
	// Recursively update all child nodes
	XMMATRIX combineWorldTransformation = XMLoadFloat4x4(&_combinedWorldTransformation);
	for (SceneGraphIterator listIterator = begin(_children);
		listIterator != end(_children);
		listIterator++)
	{
		(*listIterator)->Update(combineWorldTransformation);
	}
}

void SceneGraph::Render(void)
{
	// Recursively render all child nodes
	for (SceneGraphIterator listIterator = begin(_children);
		listIterator != end(_children);
		listIterator++)
	{
		(*listIterator)->Render();
	}
}

void SceneGraph::Shutdown(void)
{
	// Recursively shutdown all child nodes
	for (SceneGraphIterator listIterator = begin(_children);
		listIterator != end(_children);
		listIterator++)
	{
		(*listIterator)->Shutdown();
	}
}

void SceneGraph::Add(SceneNodePointer node)
{
	_children.push_back(node);
}

void SceneGraph::Remove(SceneNodePointer node)
{
	for (SceneGraphIterator listIterator = begin(_children);
		listIterator != end(_children);
		listIterator++)
	{
		// First remove this node from further down the list if it occurs there
		(*listIterator)->Remove(node);
		// If this is the node to remove, take it out of the list
		if (*listIterator == node)
		{
			_children.erase(listIterator);
		}
	}
}

SceneNodePointer SceneGraph::Find(wstring name)
{
	if (_name == name)
	{
		return shared_from_this();
	}
	SceneNodePointer returnValue = nullptr;
	for (SceneGraphIterator listIterator = begin(_children);
		listIterator != end(_children);
		listIterator++)
	{
		returnValue = (*listIterator)->Find(name);
	}
	return returnValue;
}
