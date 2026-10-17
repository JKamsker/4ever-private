-- 018_remove_duplicate_red_rocket
--
-- The first 017 run imported 4Genesis' two "The Red Rocket" pets (105 and 125) as mounts 136 and 137, but they are the
-- same mount (same mesh, textures and look under two object ids). Tools/GenesisPort now imports only pet 125, as mount
-- 136 (monster 31978, scroll 31412); 137 / 31979 and the synthesized scroll 31425 are gone from the client.
-- On a DB that ran the old 017: drop mount 137, monster 31979, item 31425 (and owned copies), point scroll 31412 at
-- mount 136, and move accounts' pet 137 to 136. No-op on a DB that only ran the current 017.
-- NOTE: chart tables load once at startup - RESTART TMapSvr/TWorldSvr after applying.
USE [TGame_gsp];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

BEGIN TRAN;

UPDATE dbo.TITEMCHART SET wUseValue = 136 WHERE wItemID = 31412 AND wUseValue = 137;

-- accounts that owned 137 keep a Red Rocket
UPDATE p SET wPetID = 136
FROM dbo.TPETTABLE p
WHERE p.wPetID = 137 AND NOT EXISTS (SELECT 1 FROM dbo.TPETTABLE q WHERE q.dwUserID = p.dwUserID AND q.wPetID = 136);
DELETE FROM dbo.TPETTABLE WHERE wPetID = 137;

DELETE FROM dbo.TITEMTABLE WHERE wItemID = 31425;
DELETE FROM dbo.TITEMCHART WHERE wItemID = 31425;
DELETE FROM dbo.TMOUNTCHART WHERE wMountID = 137;
DELETE FROM dbo.TMONSTERCHART WHERE wID = 31979;

COMMIT;
GO
