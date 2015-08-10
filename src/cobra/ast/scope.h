#ifndef SCOPE_H_
#define SCOPE_H_

#include <string>
#include <map>
#include "ast.h"
#include "cobra/types/type.h"
#include "cobra/globals.h"

namespace Cobra {
namespace internal{
	class ASTNode;
	class Type;

	class Scope
	{
	private:
		std::map<std::string, ASTNode*> objects;
		std::vector<ASTNode*> ordered;

		std::map<std::string, Type*> runtime;

		int count;
	public:
		Scope();
		~Scope();
		Scope* outer;

		Scope* NewScope();
		ASTNode* Lookup(std::string name);
		void Insert(ASTNode* node);
		void String();
		ASTNode* Get(int index);
		int Size();

		void InsertType(Type* type);
		Type* LookupType(std::string name);
		void InsertBefore(ASTNode* node);
	};
} // namespace internal
}

#endif // SCOPE_H_
