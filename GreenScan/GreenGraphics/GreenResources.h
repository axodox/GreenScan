#pragma once
#pragma managed
using namespace System;
using namespace System::Resources;
using namespace System::Reflection;

namespace Green
{
	static ref class GreenResources
	{
	private:
		static ResourceManager^ Manager;
		static GreenResources()
		{
			Assembly^ assembly = Assembly::GetExecutingAssembly();
			Manager = gcnew ResourceManager(assembly->GetName()->Name + ".Resources", assembly);
		}

	public:
		static String^ GetString(String^ key)
		{
			return Manager->GetString(key);
		}
	};
}
