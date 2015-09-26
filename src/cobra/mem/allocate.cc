#include "allocate.h"

namespace Cobra {
namespace internal{

	void* Allocate::New(size_t size){
		void* addr = malloc(size);
		if (addr == NULL) OutOfMemory();
		return addr;
	}

	void Allocate::Delete(void* ptr){
		free(ptr);
	}

	void Allocate::OutOfMemory(){
		printf("Out of Memory\n");
		exit(1);
	}

	void Allocate::Test(){
		MemoryPool* p = new MemoryPool(DEFAULT_MEMORY_POOL_SIZE, sizeof(Token), sizeof(Token) * 2);
		for (int i = 0; i < 10; i++){
			void* t = p->GetMemory(sizeof(Token));
			p->FreeMemory(t, sizeof(t));
		}
		printf("d: Total Size: %lu\n", p->GetTotalSize());
		printf("d: Used Size: %lu\n", p->GetUsedSize());
		delete p;
	}

	MemoryPool::MemoryPool(const size_t poolSize, const size_t chunkSize, const size_t memorySize){
		kChunkSize = chunkSize;
		kFirstChunk = NULL;
		kLastChunk = NULL;
		kCursorChunk = NULL;
		kTotalSize = 0;
		kUsedSize = 0;
		kFreeSize = 0;
		kMinSize = memorySize;
		kChunkCount = 0;
		AllocateMemory(poolSize);
	}

	MemoryPool::~MemoryPool(){
		FreeAllAllocatedMemory();
		DeallocateAllChunks();
	}

	void MemoryPool::DeallocateAllChunks(){
		Chunk* chunk = kFirstChunk;
	  Chunk* chunkToDelete = NULL;
	  while(chunk != NULL){
			if(chunk->isAllocationChunk){	
				if (chunkToDelete){
					free((void*) chunkToDelete);
				}
				chunkToDelete = chunk;
			}
			chunk = chunk->next;
	  }
	}

	void MemoryPool::FreeAllAllocatedMemory(){
		Chunk* chunk = kFirstChunk;
		while (chunk != NULL){
			if (chunk->isAllocationChunk){
				free((void*) chunk->data);
			}
			chunk = chunk->next;
		}
	}

	void MemoryPool::FreeMemory(void* ptr, const size_t size){
		Chunk* chunk = FindChunkHoldingPointerTo(ptr);
		if (chunk != NULL){
			FreeChunks(chunk);
		}
		kChunkCount--;
	}

	void MemoryPool::FreeChunks(Chunk* chunk){
		unsigned int kChunkCount = CalculateNeededChunks(chunk->used);
		for (unsigned int i = 0; i < kChunkCount; i++){
			if (chunk != NULL){
				chunk->used = 0;
				kUsedSize -= kChunkSize;
				chunk = chunk->next;
			}
		}
	}

	Chunk* MemoryPool::FindChunkHoldingPointerTo(void* ptr){
		Chunk* chunk = kFirstChunk;
		while (chunk != NULL){
			if (chunk->data == ((byte*) ptr)) break;
			chunk = chunk->next;
		}
		return chunk;
	}

	Chunk* MemoryPool::FindChunkSuitableToHoldMemory(const size_t size){
		unsigned int chunkToSkip = 0;
		Chunk* chunk = kCursorChunk;
		for (unsigned int i = 0; i < kChunkCount; i++){
			if (chunk != NULL){
				if (chunk == kLastChunk){
					chunk = kFirstChunk;
				}
				if (chunk->size >= size && chunk->used == 0){
					kCursorChunk = chunk;
					return chunk;
				}
				chunkToSkip = CalculateNeededChunks(chunk->used);
				if (chunkToSkip == 0) chunkToSkip = 1;
				chunk = SkipChunks(chunk, chunkToSkip);
			}
		}
		return NULL;
	}

	Chunk* MemoryPool::SkipChunks(Chunk* chunk, unsigned int chunkToSkip){
		Chunk* currentChunk = chunk;
		for (unsigned int i = 0; i < chunkToSkip; i++){
			if (currentChunk != NULL){
				currentChunk = currentChunk->next;
			}
			else{
				break;
			}
		}
		return currentChunk;
	}

	void* MemoryPool::GetMemory(const size_t size){
		size_t bestBlockSize = CalculateBestMemoryBlockSize(size);
		Chunk* chunk = NULL;
		int c = 0;
		while (chunk == NULL){
			chunk = FindChunkSuitableToHoldMemory(bestBlockSize);
			if (chunk == NULL){
				bestBlockSize = MaxValue(bestBlockSize, CalculateBestMemoryBlockSize(kMinSize));
				AllocateMemory(bestBlockSize);
			}
		}
		kUsedSize += bestBlockSize;
		kFreeSize -= bestBlockSize;
		SetMemoryChunkValues(chunk, bestBlockSize);
		return (void*) chunk->data;
	}

	void MemoryPool::SetMemoryChunkValues(Chunk* chunk, const size_t size){
		if (chunk != NULL){
			chunk->used = size;
		}
	}

	void MemoryPool::AllocateMemory(const size_t size){
		size_t bestBlockSize = CalculateBestMemoryBlockSize(size);
		unsigned int neededChunks = CalculateNeededChunks(size);
		byte* ptrNewMemBlock = (byte*) malloc(bestBlockSize);
		Chunk* ptrNewChunks = (Chunk*) malloc(neededChunks * sizeof(Chunk));
		if (ptrNewMemBlock == NULL) Allocate::OutOfMemory();
		if (ptrNewChunks == NULL) Allocate::OutOfMemory();
		kTotalSize += bestBlockSize;
		kFreeSize += bestBlockSize;
		kChunkCount += neededChunks;
		LinkChunksToData(ptrNewChunks, neededChunks, ptrNewMemBlock);
	}

	size_t MemoryPool::CalculateBestMemoryBlockSize(const size_t size){
		unsigned int needed = CalculateNeededChunks(size);
		return (size_t) (needed * kChunkSize);
	}

	unsigned int MemoryPool::CalculateNeededChunks(const size_t size){
		float f = (float) ((float) size / (float) kChunkSize);
		return ((unsigned int) ceil(f));
	}

	bool MemoryPool::LinkChunksToData(Chunk* ptrNewChunk, unsigned int chunkCount, byte* ptrNewMemBlock){
		Chunk* newChunk = NULL;
		unsigned int memoryOffset = 0;
		bool allocationChunkAssigned = false;
		for (unsigned int i = 0; i < chunkCount; i++){
			if (kFirstChunk == NULL){
				kFirstChunk = SetChunkDefaults(&ptrNewChunk[0]);
				kLastChunk = kFirstChunk;
				kCursorChunk = kFirstChunk;
			}
			else{
				newChunk = SetChunkDefaults(&ptrNewChunk[i]);
				kLastChunk->next = newChunk;
				kLastChunk = newChunk;
			}
			memoryOffset = (i * ((unsigned int) kChunkSize));
			kLastChunk->data = &(ptrNewMemBlock[memoryOffset]);

			if (!allocationChunkAssigned){
				kLastChunk->isAllocationChunk = true;
				allocationChunkAssigned = true;
			}
		}
		return RecalcChunkMemorySize(kFirstChunk, kChunkCount);
	}

	Chunk* MemoryPool::SetChunkDefaults(Chunk* chunk){
		if (chunk != NULL){
			chunk->data = NULL;
	    chunk->size = 0;
	    chunk->used = 0;
			chunk->isAllocationChunk = false;
	    chunk->next = NULL;
		}
		return chunk;
	}

	bool MemoryPool::RecalcChunkMemorySize(Chunk* chunk, unsigned int size){
		unsigned int memoryOffset = 0;
		for (unsigned int i = 0; i < size; i++){
			if (chunk){
				memoryOffset = (i * ((unsigned int) kChunkSize));
				chunk->size = (((unsigned int) kTotalSize) - memoryOffset);
				chunk = chunk->next;
			}
			else{
				return false;
			}
		}
		return true;
	}

	size_t MemoryPool::MaxValue(const size_t a, const size_t b){
		if (a > b){
			return a;
		}
		return b;
	}

} // namespace internal
} // namespace Cobra