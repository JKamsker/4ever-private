-- 017_genesis_mounts_costumes
--
-- Mounts + costumes imported from the 4Genesis client (Tools/GenesisPort `port --apply` - generated, do not edit by hand).
--   TMONSTERCHART: 13 mount recall monsters (31966, 31967, 31968, 31969, 31970, 31971, 31972, 31973, 31974, 31975, 31976, 31977, 31978), clones of 31025 (Brown Horse).
--   TMOUNTCHART:   13 mounts (124-136), plain = saddled monster (Genesis mounts have no saddle variant).
--   TITEMCHART:    13 mount items (clones of 30261) + 133 costumes
--                  (hats clone 12059, outfits 12061, wings 25004); client-file fields overwritten.
-- NOTE: chart tables load once at startup - RESTART TMapSvr/TWorldSvr after applying.
-- Idempotent: every insert is guarded by IF NOT EXISTS.
USE [TGame_gsp];
GO

SET NOCOUNT ON;
GO

-- 31966  Swift Spectral Tiger
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31966) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31966;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31967  Grey Mount
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31967) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31967;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31968  Magic Carpet Mount
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31968) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31968;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31969  Mechanical Mount
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31969) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31969;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31970  Snow Kitsune (Classic)
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31970) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31970;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31971  Magical Fish
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31971) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31971;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31972  Wooden Rudolph (Classic II)
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31972) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31972;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31973  Lightning Kitsune
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31973) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31973;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31974  Ocean Kitsune
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31974) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31974;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31975  Cloud Unicorn
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31975) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31975;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31976  Heart Ratha Panda
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31976) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31976;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31977  Heavenly Onyx Cloud Serpent
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31977) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31977;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO
-- 31978  The Red Rocket
IF NOT EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31978) AND EXISTS (SELECT 1 FROM dbo.TMONSTERCHART WHERE wID = 31025)
BEGIN
    SELECT * INTO #m FROM dbo.TMONSTERCHART WHERE wID = 31025;
    UPDATE #m SET wID = 31978;
    INSERT INTO dbo.TMONSTERCHART SELECT * FROM #m;
    DROP TABLE #m;
END
GO

IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 124) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (124, 31966, 31966);   -- Swift Spectral Tiger
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 125) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (125, 31967, 31967);   -- Grey Mount
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 126) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (126, 31968, 31968);   -- Magic Carpet Mount
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 127) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (127, 31969, 31969);   -- Mechanical Mount
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 128) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (128, 31970, 31970);   -- Snow Kitsune (Classic)
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 129) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (129, 31971, 31971);   -- Magical Fish
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 130) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (130, 31972, 31972);   -- Wooden Rudolph (Classic II)
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 131) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (131, 31973, 31973);   -- Lightning Kitsune
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 132) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (132, 31974, 31974);   -- Ocean Kitsune
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 133) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (133, 31975, 31975);   -- Cloud Unicorn
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 134) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (134, 31976, 31976);   -- Heart Ratha Panda
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 135) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (135, 31977, 31977);   -- Heavenly Onyx Cloud Serpent
IF NOT EXISTS (SELECT 1 FROM dbo.TMOUNTCHART WHERE wMountID = 136) INSERT INTO dbo.TMOUNTCHART (wMountID, wDefMonID, wUpgMonID) VALUES (136, 31978, 31978);   -- The Red Rocket
GO

-- 31412  The Red Rocket Scroll  (mount 136, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31412) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31412, szNAME=N'The Red Rocket Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=136, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31413  Grey Mount Scroll  (mount 125, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31413) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31413, szNAME=N'Grey Mount Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=125, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31414  Magic Carpet Mount Scroll  (mount 126, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31414) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31414, szNAME=N'Magic Carpet Mount Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=126, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31415  Mechanical Mount Scroll  (mount 127, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31415) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31415, szNAME=N'Mechanical Mount Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=127, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31416  Lightning Kitsune Scroll  (mount 131, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31416) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31416, szNAME=N'Lightning Kitsune Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=131, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31417  Ocean Kitsune Scroll  (mount 132, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31417) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31417, szNAME=N'Ocean Kitsune Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=132, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31418  Cloud Unicorn Scroll  (mount 133, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31418) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31418, szNAME=N'Cloud Unicorn Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=133, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31419  Heart Ratha Panda Scroll  (mount 134, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31419) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31419, szNAME=N'Heart Ratha Panda Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=134, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31420  Wooden Rudolph (Classic II) Scroll  (mount 130, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31420) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31420, szNAME=N'Wooden Rudolph (Classic II) Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=130, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=0, wUseTime=0, bUseType=66, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31421  Snow Kitsune (Classic) Scroll  (mount 128, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31421) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31421, szNAME=N'Snow Kitsune (Classic) Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=128, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=66, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31422  Magical Fish Scroll  (mount 129, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31422) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31422, szNAME=N'Magical Fish Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=129, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=0, wUseTime=0, bUseType=66, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31423  Swift Spectral Tiger Scroll  (mount 124, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31423) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31423, szNAME=N'Swift Spectral Tiger Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=124, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 31424  Heavenly Onyx Cloud Serpent Scroll  (mount 135, clone of 30261)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 31424) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 30261)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 30261;
    UPDATE #t SET wItemID=31424, szNAME=N'Heavenly Onyx Cloud Serpent Scroll', bType=12, bKind=23, wAttrID=0, wUseValue=135, dwSlotID=0, dwClassID=63, bPrmSlotID=255, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=250, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=64, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO

-- 12082  Dazzling Ensemble  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12082) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12082, szNAME=N'Dazzling Ensemble', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12083  Floppy Ears Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12083) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12083, szNAME=N'Floppy Ears Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12084  Chicken Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12084) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12084, szNAME=N'Chicken Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12085  Hawaiian Clothing  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12085) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12085, szNAME=N'Hawaiian Clothing', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12086  Vampire Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12086) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12086, szNAME=N'Vampire Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12087  Beer Warrior  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12087) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12087, szNAME=N'Beer Warrior', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12088  Costume Dress 1  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12088) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12088, szNAME=N'Costume Dress 1', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12089  Costume Dress 2  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12089) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12089, szNAME=N'Costume Dress 2', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12090  Costume Dress 3  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12090) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12090, szNAME=N'Costume Dress 3', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12091  Costume Dress 4  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12091) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12091, szNAME=N'Costume Dress 4', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12092  Costume Dress 5  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12092) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12092, szNAME=N'Costume Dress 5', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12093  Costume Dress 6  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12093) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12093, szNAME=N'Costume Dress 6', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12094  Costume Dress 7  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12094) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12094, szNAME=N'Costume Dress 7', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12095  Costume Dress 8  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12095) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12095, szNAME=N'Costume Dress 8', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12096  Costume Dress 9  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12096) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12096, szNAME=N'Costume Dress 9', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12097  Costume Dress 10  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12097) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12097, szNAME=N'Costume Dress 10', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12098  Costume Dress 11  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12098) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12098, szNAME=N'Costume Dress 11', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12099  Costume Dress 12  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12099) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12099, szNAME=N'Costume Dress 12', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12100  Costume Dress 13  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12100) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12100, szNAME=N'Costume Dress 13', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12101  Costume Dress 14  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12101) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12101, szNAME=N'Costume Dress 14', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12102  Costume Dress 15  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12102) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12102, szNAME=N'Costume Dress 15', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12103  Costume Dress 16  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12103) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12103, szNAME=N'Costume Dress 16', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12104  Costume Dress 17  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12104) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12104, szNAME=N'Costume Dress 17', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12105  Costume Dress 18  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12105) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12105, szNAME=N'Costume Dress 18', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12106  Costume Dress 19  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12106) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12106, szNAME=N'Costume Dress 19', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12107  Costume Dress 20  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12107) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12107, szNAME=N'Costume Dress 20', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12108  Costume Dress 21  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12108) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12108, szNAME=N'Costume Dress 21', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12109  Costume Dress 22  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12109) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12109, szNAME=N'Costume Dress 22', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12110  Plague Master's Garb (Brown)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12110) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12110, szNAME=N'Plague Master''s Garb (Brown)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12111  Costume Dress 24  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12111) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12111, szNAME=N'Costume Dress 24', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12112  Plague Master's Garb (Blue)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12112) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12112, szNAME=N'Plague Master''s Garb (Blue)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12113  Costume Dress 26  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12113) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12113, szNAME=N'Costume Dress 26', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12114  Costume Dress 27  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12114) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12114, szNAME=N'Costume Dress 27', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12115  Costume Dress 28  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12115) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12115, szNAME=N'Costume Dress 28', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12116  Costume Dress 29  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12116) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12116, szNAME=N'Costume Dress 29', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12117  DBZ Outfit  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12117) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12117, szNAME=N'DBZ Outfit', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12118  Snowsuit  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12118) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12118, szNAME=N'Snowsuit', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12119  Costume Dress 32  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12119) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12119, szNAME=N'Costume Dress 32', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12120  Costume Hat 1  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12120) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12120, szNAME=N'Costume Hat 1', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12121  Costume Hat 2  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12121) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12121, szNAME=N'Costume Hat 2', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12122  Costume Hat 3  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12122) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12122, szNAME=N'Costume Hat 3', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12123  Costume Hat 4  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12123) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12123, szNAME=N'Costume Hat 4', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12124  Costume Hat 5  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12124) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12124, szNAME=N'Costume Hat 5', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12125  Costume Hat 6  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12125) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12125, szNAME=N'Costume Hat 6', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12126  Costume Hat 7  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12126) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12126, szNAME=N'Costume Hat 7', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12127  Costume Hat 8  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12127) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12127, szNAME=N'Costume Hat 8', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12128  Costume Hat 9  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12128) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12128, szNAME=N'Costume Hat 9', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12129  Costume Hat 10  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12129) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12129, szNAME=N'Costume Hat 10', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12130  Costume Hat 11  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12130) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12130, szNAME=N'Costume Hat 11', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12131  Costume Hat 12  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12131) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12131, szNAME=N'Costume Hat 12', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12132  Costume Hat 13  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12132) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12132, szNAME=N'Costume Hat 13', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12133  Pink Cat Headset  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12133) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12133, szNAME=N'Pink Cat Headset', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12134  White Cat Ear Headset  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12134) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12134, szNAME=N'White Cat Ear Headset', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12135  Costume Hat 16  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12135) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12135, szNAME=N'Costume Hat 16', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12136  Costume Hat 17  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12136) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12136, szNAME=N'Costume Hat 17', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12137  Plague Mask (Brown)  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12137) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12137, szNAME=N'Plague Mask (Brown)', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12138  Plague Mask (Blue)  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12138) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12138, szNAME=N'Plague Mask (Blue)', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12139  Costume Hat 20  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12139) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12139, szNAME=N'Costume Hat 20', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12140  Costume Hat 21  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12140) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12140, szNAME=N'Costume Hat 21', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12141  Costume Hat 22  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12141) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12141, szNAME=N'Costume Hat 22', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12142  Costume Hat 23  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12142) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12142, szNAME=N'Costume Hat 23', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12143  Costume Hat 24  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12143) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12143, szNAME=N'Costume Hat 24', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12144  Costume Hat 25  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12144) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12144, szNAME=N'Costume Hat 25', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12145  Lil' Gadget  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12145) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12145, szNAME=N'Lil'' Gadget', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12146  Snowman Helmet  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12146) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12146, szNAME=N'Snowman Helmet', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12147  Seraph Wings  (wings, clone of 25004)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12147) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 25004)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 25004;
    UPDATE #t SET wItemID=12147, szNAME=N'Seraph Wings', bType=17, bKind=250, wAttrID=21101, wUseValue=0, dwSlotID=131072, dwClassID=-1, bPrmSlotID=17, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=5, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12148  Fae Wings  (wings, clone of 25004)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12148) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 25004)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 25004;
    UPDATE #t SET wItemID=12148, szNAME=N'Fae Wings', bType=17, bKind=250, wAttrID=21101, wUseValue=0, dwSlotID=131072, dwClassID=-1, bPrmSlotID=17, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=5, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12149  Dread Wings  (wings, clone of 25004)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12149) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 25004)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 25004;
    UPDATE #t SET wItemID=12149, szNAME=N'Dread Wings', bType=17, bKind=250, wAttrID=21101, wUseValue=0, dwSlotID=131072, dwClassID=-1, bPrmSlotID=17, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=5, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12150  Prism Wings  (wings, clone of 25004)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12150) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 25004)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 25004;
    UPDATE #t SET wItemID=12150, szNAME=N'Prism Wings', bType=17, bKind=250, wAttrID=21101, wUseValue=0, dwSlotID=131072, dwClassID=-1, bPrmSlotID=17, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=5, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12151  Phoenix Wings  (wings, clone of 25004)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12151) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 25004)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 25004;
    UPDATE #t SET wItemID=12151, szNAME=N'Phoenix Wings', bType=17, bKind=250, wAttrID=21101, wUseValue=0, dwSlotID=131072, dwClassID=-1, bPrmSlotID=17, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=5, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12152  Seraph Wings  (wings, clone of 25004)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12152) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 25004)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 25004;
    UPDATE #t SET wItemID=12152, szNAME=N'Seraph Wings', bType=17, bKind=250, wAttrID=21101, wUseValue=0, dwSlotID=131072, dwClassID=-1, bPrmSlotID=17, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=5, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=0, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12153  Visage of the Blind  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12153) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12153, szNAME=N'Visage of the Blind', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12154  Pumpkin Helmet  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12154) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12154, szNAME=N'Pumpkin Helmet', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12155  Pumpkin Helmet  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12155) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12155, szNAME=N'Pumpkin Helmet', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12156  Gothic Rabbit Hat (Noir)  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12156) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12156, szNAME=N'Gothic Rabbit Hat (Noir)', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12157  Gothic Floppy Ears Costume (Noir)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12157) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12157, szNAME=N'Gothic Floppy Ears Costume (Noir)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12158  Gothic Rabbit Hat (Rose)  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12158) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12158, szNAME=N'Gothic Rabbit Hat (Rose)', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12159  Gothic Floppy Ears Costume (Rose)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12159) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12159, szNAME=N'Gothic Floppy Ears Costume (Rose)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12160  Gothic Bunny  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12160) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12160, szNAME=N'Gothic Bunny', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12161  Black Goku Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12161) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12161, szNAME=N'Black Goku Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12162  Firebender Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12162) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12162, szNAME=N'Firebender Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12163  Earthbender Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12163) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12163, szNAME=N'Earthbender Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12164  Dazzling Ensemble  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12164) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12164, szNAME=N'Dazzling Ensemble', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12165  Rabbit Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12165) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12165, szNAME=N'Rabbit Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12166  Floppy Ears Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12166) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12166, szNAME=N'Floppy Ears Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12167  Chicken Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12167) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12167, szNAME=N'Chicken Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12168  Chicken Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12168) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12168, szNAME=N'Chicken Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12169  Hawaiian Headdress  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12169) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12169, szNAME=N'Hawaiian Headdress', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12170  Hawaiian Clothing  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12170) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12170, szNAME=N'Hawaiian Clothing', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12171  Delightful Outfit  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12171) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12171, szNAME=N'Delightful Outfit', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12172  Snorkel  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12172) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12172, szNAME=N'Snorkel', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12173  Beer Warrior  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12173) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12173, szNAME=N'Beer Warrior', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12174  Vampire Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12174) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12174, szNAME=N'Vampire Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12175  Vampire Mask  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12175) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12175, szNAME=N'Vampire Mask', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12176  Elite's Dress  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12176) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12176, szNAME=N'Elite''s Dress', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12177  Swimwear (Holy Knight Collection)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12177) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12177, szNAME=N'Swimwear (Holy Knight Collection)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12178  Swimwear (Combatant Collection)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12178) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12178, szNAME=N'Swimwear (Combatant Collection)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12179  Swimwear (Warlord Collection)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12179) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12179, szNAME=N'Swimwear (Warlord Collection)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12180  Protective Shirt (Holy Knight Collection)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12180) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12180, szNAME=N'Protective Shirt (Holy Knight Collection)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12181  Protective Shirt (Combatant Collection)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12181) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12181, szNAME=N'Protective Shirt (Combatant Collection)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12182  Protective Shirt (Warlord Collection)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12182) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12182, szNAME=N'Protective Shirt (Warlord Collection)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12183  Corrupted Servant's Uniforn  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12183) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12183, szNAME=N'Corrupted Servant''s Uniforn', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12184  Corrupted Festive Dress  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12184) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12184, szNAME=N'Corrupted Festive Dress', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12185  Jason's Mask  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12185) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12185, szNAME=N'Jason''s Mask', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12186  Corrupted Pumpkin Helmet  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12186) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12186, szNAME=N'Corrupted Pumpkin Helmet', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12187  Conquerors' Garment  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12187) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12187, szNAME=N'Conquerors'' Garment', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12188  Rumbling Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12188) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12188, szNAME=N'Rumbling Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12189  Icy Reindeer Cap  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12189) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12189, szNAME=N'Icy Reindeer Cap', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12190  Icy Worn Out Christmas Feline Beanie  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12190) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12190, szNAME=N'Icy Worn Out Christmas Feline Beanie', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12191  Icy Father Christmas Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12191) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12191, szNAME=N'Icy Father Christmas Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12192  Candy Cane Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12192) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12192, szNAME=N'Candy Cane Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12193  Penguin Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12193) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12193, szNAME=N'Penguin Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12194  Penguin Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12194) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12194, szNAME=N'Penguin Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12195  Icy Christmas Spirits Mask  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12195) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12195, szNAME=N'Icy Christmas Spirits Mask', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12196  Minun Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12196) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12196, szNAME=N'Minun Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12197  Minun Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12197) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12197, szNAME=N'Minun Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12198  Plusle Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12198) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12198, szNAME=N'Plusle Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12199  Plusle Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12199) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12199, szNAME=N'Plusle Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12200  Valentines Dress  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12200) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12200, szNAME=N'Valentines Dress', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12201  Cute Christmas Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12201) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12201, szNAME=N'Cute Christmas Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12202  Cute Worn Out Christmas Beanie  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12202) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12202, szNAME=N'Cute Worn Out Christmas Beanie', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12203  Grinch Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12203) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12203, szNAME=N'Grinch Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12204  Grinch Beanie  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12204) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12204, szNAME=N'Grinch Beanie', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12205  Worn Out Grinch Beanie  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12205) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12205, szNAME=N'Worn Out Grinch Beanie', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12206  Golden Dress  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12206) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12206, szNAME=N'Golden Dress', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12207  Undead Rabbit Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12207) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12207, szNAME=N'Undead Rabbit Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12208  Undead Floppy Ears Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12208) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12208, szNAME=N'Undead Floppy Ears Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12209  Lifeguard Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12209) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12209, szNAME=N'Lifeguard Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12210  Vampire Costume (Black)  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12210) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12210, szNAME=N'Vampire Costume (Black)', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12211  Vampire Mask (Black)  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12211) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12211, szNAME=N'Vampire Mask (Black)', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12212  Jolly Bunny Costume  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12212) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12212, szNAME=N'Jolly Bunny Costume', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12213  Jolly Bunny Hat  (hat, clone of 12059)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12213) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12059)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12059;
    UPDATE #t SET wItemID=12213, szNAME=N'Jolly Bunny Hat', bType=17, bKind=88, wAttrID=0, wUseValue=0, dwSlotID=32768, dwClassID=63, bPrmSlotID=15, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
-- 12214  Fancy Festive Dress  (outfit, clone of 12061)
IF NOT EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12214) AND EXISTS (SELECT 1 FROM dbo.TITEMCHART WHERE wItemID = 12061)
BEGIN
    SELECT * INTO #t FROM dbo.TITEMCHART WHERE wItemID = 12061;
    UPDATE #t SET wItemID=12214, szNAME=N'Fancy Festive Dress', bType=17, bKind=89, wAttrID=0, wUseValue=0, dwSlotID=65536, dwClassID=63, bPrmSlotID=16, bSubSlotID=255, bLevel=1, bCanRepair=0, dwDuraMax=0, bRefineMax=0, bMinRange=0, bMaxRange=0, bStack=1, bSlotCount=0, bCanGamble=0, bGambleProb=0, bDestroyProb=0, bCanGrade=0, bCanMagic=0, bCanRare=0, wDelayGroupID=0, dwDelay=0, bIsSpecial=1, wUseTime=0, bUseType=0, bCanWrap=0, dwCode=1048783, bCanColor=0;
    INSERT INTO dbo.TITEMCHART SELECT * FROM #t;
    DROP TABLE #t;
END
GO
