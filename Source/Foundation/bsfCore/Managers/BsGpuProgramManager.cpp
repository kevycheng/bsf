//************************************ bs::framework - Copyright 2018 Marko Pintera **************************************//
//*********** Licensed under the MIT license. See LICENSE.md for full terms. This notice is not to be removed. ***********//
#include "Managers/BsGpuProgramManager.h"
#include "RenderAPI/BsRenderAPI.h"

namespace bs 
{
	SPtr<GpuProgram> GpuProgramManager::create(const GPU_PROGRAM_DESC& desc)
	{
		GpuProgram* program = new (bs_alloc<GpuProgram>()) GpuProgram(desc);
		SPtr<GpuProgram> ret = bs_core_ptr<GpuProgram>(program);
		ret->_setThisPtr(ret);
		ret->initialize();

		return ret;
	}

	SPtr<GpuProgram> GpuProgramManager::createEmpty(const String& language, GpuProgramType type)
	{
		GPU_PROGRAM_DESC desc;
		desc.language = language;
		desc.type = type;

		GpuProgram* program = new (bs_alloc<GpuProgram>()) GpuProgram(desc);
		SPtr<GpuProgram> ret = bs_core_ptr<GpuProgram>(program);
		ret->_setThisPtr(ret);

		return ret;
	}

	namespace ct
	{
	String sNullLang = "null";

	/** Null GPU program used in place of GPU programs we cannot create. Null programs don't do anything. */
	class NullProgram : public GpuProgram
	{
	public:
		NullProgram()
			:GpuProgram(GPU_PROGRAM_DESC(), GDF_DEFAULT)
		{ }

		~NullProgram() { }

		bool isSupported() const { return false; }
		const String& getLanguage() const { return sNullLang; }

	protected:
		void loadFromSource() {}

		void buildConstantDefinitions() const { }
	};

	/**	Factory that creates null GPU programs.  */
	class NullProgramFactory : public GpuProgramFactory
	{
	public:
		NullProgramFactory() {}
		~NullProgramFactory() {}

		SPtr<GpuProgram> create(const GPU_PROGRAM_DESC& desc, GpuDeviceFlags deviceMask) override
		{
			SPtr<NullProgram> ret = bs_shared_ptr_new<NullProgram>();
			ret->_setThisPtr(ret);

			return ret;
		}

		SPtr<GpuProgram> create(GpuProgramType type, GpuDeviceFlags deviceMask) override
		{
			SPtr<NullProgram> ret = bs_shared_ptr_new<NullProgram>();
			ret->_setThisPtr(ret);

			return ret;
		}

		GpuProgramCompileStatus compile(const GPU_PROGRAM_DESC& desc) override
		{
			GpuProgramCompileStatus ret;
			ret.success = false;
			ret.program.instructions.data = nullptr;
			ret.program.instructions.size = 0;
			ret.program.machineSpecific = false;
			
			return ret;
		}
	};

	GpuProgramManager::GpuProgramManager()
	{
		mNullFactory = bs_new<NullProgramFactory>();
		addFactory(sNullLang, mNullFactory);
	}

	GpuProgramManager::~GpuProgramManager()
	{
		bs_delete((NullProgramFactory*)mNullFactory);
	}

	void GpuProgramManager::addFactory(const String& language, GpuProgramFactory* factory)
	{
		Lock(mMutex);

		mFactories[language] = factory;
	}

	void GpuProgramManager::removeFactory(const String& language)
	{
		Lock(mMutex);

		auto iter = mFactories.find(language);
		if (iter != mFactories.end())
			mFactories.erase(iter);
	}

	GpuProgramFactory* GpuProgramManager::getFactory(const String& language)
	{
		auto iter = mFactories.find(language);
		if (iter == mFactories.end())
			iter = mFactories.find(sNullLang);

		return iter->second;
	}

	bool GpuProgramManager::isLanguageSupported(const String& lang)
	{
		Lock(mMutex);

		auto iter = mFactories.find(lang);
		return iter != mFactories.end();
	}

	SPtr<GpuProgram> GpuProgramManager::create(const GPU_PROGRAM_DESC& desc, GpuDeviceFlags deviceMask)
	{
		SPtr<GpuProgram> ret = createInternal(desc, deviceMask);
		ret->initialize();

		return ret;
	}

	SPtr<GpuProgram> GpuProgramManager::createInternal(const GPU_PROGRAM_DESC& desc, GpuDeviceFlags deviceMask)
	{
		GpuProgramFactory* factory = getFactory(desc.language);
		SPtr<GpuProgram> ret = factory->create(desc, deviceMask);

		return ret;
	}

	GpuProgramCompileStatus GpuProgramManager::compile(const GPU_PROGRAM_DESC& desc)
	{
		GpuProgramFactory* factory = getFactory(desc.language);
		return factory->compile(desc);
	}
	}
}
