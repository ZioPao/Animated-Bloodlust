modded class SCR_CharacterDamageManagerComponent : ScriptedDamageManagerComponent
{
	ref map<string, string> bsSettings;
	
	const string BS_FileNameJson = "BS_Settings.json";
	const string BS_MOD_ID = "59951797A291CA02";				
	const ResourceName weaponSplatterMaterial = "{098960A4823D679F}materials/weapon_splatter/WeaponBlood.emat";

	IEntity currentCharacter;
	bool alreadyDestroyed = false;
	float timerBetweenSplatters; 
	World world;

	
	ref static array<ref DecalWrapper> currentPlayerDecals;
	ref array<ref DecalWrapper> currentCharacterDecals;

	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		currentCharacter = owner;
		world = owner.GetWorld();
		
		currentCharacterDecals = new ref array<ref DecalWrapper>();
		
		if (!currentPlayerDecals)
			currentPlayerDecals = new array<ref DecalWrapper>();
		

		////////////////////////////////////////////////////////////////////////////////////////////////
		//Settings initialization stuff 
		MCF_SettingsManager BS_mcfSettingsManager = MCF_SettingsManager.GetInstance();
		OrderedVariablesMap bsVariablesMap = new OrderedVariablesMap();
				
		bsVariablesMap.Set("waitTimeBetweenFrames", new VariableInfo("Wait between frames", "0.033", EFilterType.TYPE_FLOAT));
		bsVariablesMap.Set("enableWeaponSplatters", new VariableInfo("Enable Weapon Splatters (Currently kinda broken)", "0", EFilterType.TYPE_FLOAT));
		
		
		bsVariablesMap.Set("bloodpoolMinimumAlphaMulChange", new VariableInfo("Alpha Mul Bloodpools - Min Random Change", "0.0002", EFilterType.TYPE_FLOAT));
		bsVariablesMap.Set("bloodpoolMaximumAlphaMulChange", new VariableInfo("Alpha Mul Bloodpools - Max Random Change", "0.03", EFilterType.TYPE_FLOAT));
			
		bsVariablesMap.Set("wallsplatterMinimumAlphaMulChange", new VariableInfo("Alpha Mul Wallsplatters - Min Random Change", "0.0001", EFilterType.TYPE_FLOAT));
		bsVariablesMap.Set("wallsplatterMaximumAlphaMulChange", new VariableInfo("Alpha Mul Wallsplatters - Max Random Change", "0.02", EFilterType.TYPE_FLOAT));
		
		bsVariablesMap.Set("wallsplatterMinimumAlphaTestChange", new VariableInfo("Alpha Test Wallsplatters - Min Random Change", "0.0001", EFilterType.TYPE_FLOAT));
		bsVariablesMap.Set("wallsplatterMaximumAlphaTestChange", new VariableInfo("Alpha Test Wallsplatters - Max Random Change", "0.02", EFilterType.TYPE_FLOAT));
	
			
		bsVariablesMap.Set("maxAlphaMul", new VariableInfo("Alpha Test Maximum value", "5", EFilterType.TYPE_FLOAT));			//max 5
		bsVariablesMap.Set("minAlphaTest", new VariableInfo("Alpha Test Minimum value", "0.1", EFilterType.TYPE_FLOAT));
		
		bsVariablesMap.Set("chanceStaticDecal", new VariableInfo("Chance of a static blood decal to appear (0 to 100)", "50", EFilterType.TYPE_FLOAT));

	
			
		bsVariablesMap.Set("maxDecalsPerChar", new VariableInfo("Max Decals per Character", "2", EFilterType.TYPE_INT));
		bsVariablesMap.Set("maxDecalsPlayerWeapon", new VariableInfo("Max Decals for Player Weapon", "4", EFilterType.TYPE_INT));
			
		bsVariablesMap.Set("bloodpoolSize", new VariableInfo("Bloodpol Size", "1.5", EFilterType.TYPE_FLOAT));
		bsVariablesMap.Set("wallsplatterSize", new VariableInfo("Wallsplatter Size", "1", EFilterType.TYPE_FLOAT));

		
		//bsVariablesMap.Set("debugSpheres", new VariableInfo("Debug Spheres", "0", EFilterType.TYPE_BOOL));

		
		if (!BS_mcfSettingsManager.GetJsonManager(BS_MOD_ID))
			bsSettings = BS_mcfSettingsManager.Setup(BS_MOD_ID, BS_FileNameJson, bsVariablesMap);
		else if (!bsSettings)
		{
			bsSettings = BS_mcfSettingsManager.GetModSettings(BS_MOD_ID);
			BS_mcfSettingsManager.GetJsonManager(BS_MOD_ID).SetUserHelpers(bsVariablesMap);		
		}
				
		
		//////////////////////////////////////////////////////////////////////////////////////////////////
		
		
	}

	override void OnDamage(
			EDamageType type,
			float damage,
			HitZone pHitZone,
			IEntity instigator,
			inout vector hitTransform[3],
			float speed,
			int colliderID,
			int nodeID)
	{
		super.OnDamage(type, damage, pHitZone, instigator, hitTransform, speed, colliderID, nodeID);
			
		
		
		if (MCF_SettingsManager.IsInitialized())
		{
			MCF_SettingsManager BS_mcfSettingsManager = MCF_SettingsManager.GetInstance();
			bsSettings = BS_mcfSettingsManager.GetModSettings(BS_MOD_ID);
		
			
			//Setup animatedBloodManager
			BS_AnimateBloodManager animatedBloodManager;		
			animatedBloodManager = BS_AnimateBloodManager.GetInstance();		
			if (!animatedBloodManager)
				animatedBloodManager = BS_AnimateBloodManager.Cast(GetGame().SpawnEntity(BS_AnimateBloodManager, GetGame().GetWorld(), null));
				
			
			bsSettings = MCF_SettingsManager.GetInstance().GetModSettings(BS_MOD_ID);
	

			int correctNodeId;
			int colliderDescriptorIndex = pHitZone.GetColliderDescriptorIndex(colliderID);
			pHitZone.TryGetColliderDescription(currentCharacter, colliderDescriptorIndex, null, null, correctNodeId);
			
	
			
			if (hitTransform[0].Length() != 0)
			{
				
				if (GetState() == EDamageState.DESTROYED && !alreadyDestroyed)
				{
					GetGame().GetCallqueue().CallLater(animatedBloodManager.StartNewAnimation, 2000, false, currentCharacter, hitTransform[0], hitTransform[1], EDecalType.BLOODPOOL, false, 1.5, correctNodeId);
					alreadyDestroyed = true;		//only once
				}
				else if (damage > 20.0)
				{
					//todo add a timer to prevent more than 1 splatter on the wall 
					animatedBloodManager.StartNewAnimation(currentCharacter,  hitTransform[0],  hitTransform[1], EDecalType.WALLSPLATTER, false, 0.0, correctNodeId);
	
					
				}
			
				int enableWeaponSplatters = bsSettings.Get("enableWeaponSplatters").ToInt();
				
				if (enableWeaponSplatters == 1)
					GenerateWeaponSplatters(currentCharacter, bsSettings);
				
				float chanceStaticDecal = bsSettings.Get("chanceStaticDecal").ToFloat();
				if (Math.RandomInt(0,101) < chanceStaticDecal)		
					animatedBloodManager.SpawnSingleFrame(currentCharacter, world, hitTransform[0], hitTransform[1]);
	
				
			}
		}
	}
	
	
	void GenerateWeaponSplatters(IEntity currentChar, map<string,string> settings)
	{
		
		// Settings
		float farClip = bsSettings.Get("farClip").ToFloat();
		float nearClip = bsSettings.Get("nearClip").ToFloat();
		int maxDecalsPlayerWeapon = bsSettings.Get("maxDecalsPlayerWeapon").ToInt();	
		bool debugSpheres = 0;			//settings.Get("debugSpheres").ToInt();
		
		// Other characters 
		int maxDecalsPerChar = bsSettings.Get("maxDecalsPerChar").ToInt();
		ManageWeaponDecalsStack(currentChar, currentCharacterDecals, 0, 2, maxDecalsPerChar);

		// Player Weapon
		PlayerManager pMan = GetGame().GetPlayerManager();
		SCR_PlayerController m_playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		IEntity currentPlayer = m_playerController.GetMainEntity();
		vector playerTransform[4];
		currentPlayer.GetTransform(playerTransform);		
			

		
		
		vector originCurrentCharacter = currentChar.GetOrigin();

		if (vector.Distance(playerTransform[3], originCurrentCharacter) < 3)
			ManageWeaponDecalsStack(currentPlayer, currentPlayerDecals, nearClip, farClip, maxDecalsPlayerWeapon, debugSpheres);
		
	
	}
	


	
	void ManageWeaponDecalsStack(IEntity owner, array<ref DecalWrapper> stack, float nearClip, float farClip, int maxDecals, bool debugSpheres = false)
	{
		int materialColor = Color.FromRGBA(16, 0, 0,255).PackToInt();		//move this away
		int count = stack.Count();
		
		if (count >= maxDecals)
		{
			int index = Math.RandomIntInclusive(0, stack.Count() - 1);
			//Print("Removing element " + index);
			DecalWrapper tmpWrapper = stack.Get(index);
			Decal tmpDecal = tmpWrapper.wrappedDecal;
			if (tmpDecal)
			{
				world.RemoveDecal(tmpDecal);
				stack.Remove(index);
			}

		}
		
		// Tries to generate a decal
		count = stack.Count();
		if (count < maxDecals)
		{
			
			vector leftHandTransform[4];
			int nodeLeftHand = 2089750091;
			owner.GetBoneMatrix(nodeLeftHand, leftHandTransform);

			
			vector tmpTransformHand = leftHandTransform[3];
			tmpTransformHand[0] = tmpTransformHand[0]; //+ diffOriginX;
			tmpTransformHand[1] = tmpTransformHand[1]; //+ diffOriginY;
			tmpTransformHand[2] = tmpTransformHand[2]; //+ diffOriginZ;
			
			vector origin = owner.CoordToParent(tmpTransformHand);
			vector projection = origin;

			DecalWrapper newTmpWrapperLeft = DecalWrapper(world.CreateDecal(owner, origin, projection, nearClip, farClip, 0, 1, 1, weaponSplatterMaterial, -1, materialColor));
			stack.Insert(newTmpWrapperLeft);
			if(debugSpheres)
				MCF_Debug.DrawSphereAtPos(origin, COLOR_RED);

		}

		
	}
	
	
	//------------------------------------------------------------------------------------------------

	
	static void CleanWeapon()
	{
		
		//todo spherecast and get near decals.
		//Print(previousPlayerWeaponDecal);
		//Print(currentPlayerDecals.Count());
		
		

		int count = currentPlayerDecals.Count();
		
		foreach(DecalWrapper decWrapper : currentPlayerDecals)
		{
			Decal tmpDecal = decWrapper.wrappedDecal;
			
			if (tmpDecal)
			{
				//Print("Removing decal: " + tmpDecal);
				SCR_PlayerController m_playerController = SCR_PlayerController.Cast(GetGame().GetPlayerController());
				IEntity currentPlayer = m_playerController.GetMainEntity();
				SCR_CharacterControllerComponent.Cast(currentPlayer.FindComponent(SCR_CharacterControllerComponent)).SetInspectionMode(true);
				GetGame().GetWorld().RemoveDecal(tmpDecal);		//doesn't automatically destroy the pointer

			}
		}
		
		currentPlayerDecals.Clear();

	}
	
	
	
	override void CreateBleedingParticleEffect(notnull HitZone hitZone, float bleedingRate, int colliderDescriptorIndex)
	{
		// Play Bleeding particle
		if (m_sBleedingParticle.IsEmpty())
			return;
		
		RemoveBleedingParticleEffect(hitZone);
		
		if (bleedingRate == 0 || m_fBleedingParticleRateScale == 0)
			return;
		
		// TODO: Blood traces on ground that should be left regardless of clothing, perhaps just delayed
		SCR_CharacterHitZone characterHitZone = SCR_CharacterHitZone.Cast(hitZone);
		
		//Let's just disable this.
		//if (characterHitZone.IsCovered())
		//	return;
		
		// Get bone node
		vector transform[4];
		int boneIndex;
		int boneNode;
		if (!hitZone.TryGetColliderDescription(GetOwner(), colliderDescriptorIndex, transform, boneIndex, boneNode))
			return;
		
		SCR_ParticleEmitter particleEmitter = SCR_ParticleAPI.PlayOnObjectPTC(GetOwner(), m_sBleedingParticle, vector.Zero, vector.Zero, boneNode);
		SCR_ParticleAPI.LerpAllEmitters(particleEmitter, bleedingRate * m_fBleedingParticleRateScale, EmitterParam.BIRTH_RATE);
		
		if (!m_mBleedingParticles)
			m_mBleedingParticles = new map<HitZone, SCR_ParticleEmitter>;
		
		m_mBleedingParticles.Insert(hitZone, particleEmitter);
	}
	


	

}


class DecalWrapper
{
	Decal wrappedDecal;
	
	
	void DecalWrapper(Decal d)
	{
		wrappedDecal = d;
	
	}

}