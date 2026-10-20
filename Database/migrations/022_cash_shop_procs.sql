-- 022_cash_shop_procs
--
-- The cash shop (TMapSvr.Net MapService.CashShop.cs) on the baseline's stand-in for the cash server (TGlobal_gsp.TCASHTESTTABLE):
--
-- 1. TGlobal_gsp.TCashItemBuy put only the bought item's BONUS items in the cabinet: the item itself came from the cash server's
--    Fun_BuyTheItem, which is commented out. A purchase took the cash and gave nothing. The item is now inserted too. Its cabinet
--    inserts also left out bGem and wMoggItemID, which are NOT NULL with no default, so even the bonus items failed; both are 0 now.
-- 2. TGame_gsp.TCashCabinetBuy took (@dateTime, @dwCharID), but the map calls it as the C++ does - the time, the cash and the
--    bonus out, then the shop item and the character - so buying cabinet time could never work. It now has that signature and
--    passes the shop item on.
-- 3. TGlobal_gsp.TCashCabinetBuy: RETURNed inside its open transaction (the transaction leaked), priced a sale with integer division
--    (1 - bSaleValue/100 is 1 for any sale under 100%), stored the unset @dateTime for an account with no TUSERINFOTABLE row, and
--    never set @dateTime - so the client never learnt the cabinet's new time.
-- 4. TGame_gsp.TGainCashBonus called TGlobal_gsp through the linked server [4SDBGLB], which does not exist here.
--
-- Idempotent: CREATE OR ALTER.
USE [TGlobal_gsp];
GO

CREATE OR ALTER PROCEDURE [dbo].[TCashItemBuy]
@dwCash INT OUTPUT,
@dwCashBonus INT OUTPUT,
@szPresent VARCHAR(50) OUTPUT,
@dwUserID INT,
@wCashItemID SMALLINT,
@dwMoney INT,
@bBuyType TINYINT,
@dwTarget INT
AS

DECLARE @nRet INT
DECLARE @bConsumType TINYINT 		-- buy type (0 auto, 1 cash, 2 mileage)
DECLARE @bMsgCode	TINYINT		-- error (0 none, 1 no item, 2 no cash points, 3 not enough cash)
DECLARE @dwPcBangID INT
DECLARE @dwRealTarget INT
DECLARE @szLoginIP VARCHAR(50)
DECLARE @dCurTime SMALLDATETIME
DECLARE @dSetTime SMALLDATETIME
DECLARE @wUseTime SMALLINT

SET @bConsumType = 0
SET @dwPcBangID  = 0
SET @szLoginIP = ''
SET @nRet = 0
SET @szPresent = ''
SET @dCurTime = GetDate()
SET @dwCash = 0
SET @dwCashBonus = 0
SET @wUseTime = 0

SELECT @szPresent = szName, @wUseTime = wUseTime FROM TCASHSHOPITEMCHART WHERE wID = @wCashItemID 
IF(@@ROWCOUNT <> 1 OR @szPresent IS NULL)
BEGIN
	SET @szPresent = ''
	RETURN 1
END

SELECT @dwPcBangID = dwPcBangID, @szLoginIP = szLoginIP FROM TCURRENTUSER WHERE dwUserID = @dwUserID

-- TEST TABLE############################
SELECT @dwCash = dwCash, @dwCashBonus=dwBonus FROM TCASHTESTTABLE WHERE dwUserID = @dwUserID
IF(@@ROWCOUNT < 1 OR @dwCash<@dwMoney)
	RETURN 3

SET @dwCash = @dwCash - @dwMoney
UPDATE TCASHTESTTABLE SET dwCash = @dwCash WHERE dwUserID = @dwUserID

--######################################
--EXEC @nRet = [192.168.1.9,6121].FourStory_Cash.FourStory_Web.Fun_BuyTheItem @bConsumType, @wCashItemID, @dwUserID, @dwMoney, @dwPcBangID, @bBuyType, @dwTarget, @szLoginIP, @bMsgCode OUTPUT, @dwCash OUTPUT, @dwCashBonus OUTPUT

IF(@nRet = 0)
BEGIN
	SET @dSetTime = DATEADD(MINUTE, 60-DATEPART(MINUTE, @dCurTime), @dCurTime)
--	SET @dSetTime = DATEADD(HOUR, 24-DATEPART(HOUR, @dSetTime), @dSetTime) + 1


	IF(@bBuyType = 2)
		SET @dwRealTarget = @dwTarget
	ELSE
		SET @dwRealTarget = @dwUserID

	-- 022: the bought item itself (the cash server used to put it in the cabinet; with it commented out, only the bonus
	-- items were inserted and a purchase took the cash for nothing).
	INSERT INTO TCASHITEMCABINETTABLE(
		dwUserID, wItemID, bLevel, bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime,bGradeEffect,
		bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6,
		wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
		dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, bGem, wMoggItemID)
	SELECT @dwRealTarget, wItemID, bLevel, bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur,
		CASE WHEN wUseTime > 0 THEN @dSetTime + wUseTime ELSE 0 END, bGradeEffect,
		bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6,
		wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
		dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, 0, 0
	FROM TCASHSHOPITEMCHART WHERE wID = @wCashItemID AND wItemID <> 0
	UPDATE TCASHITEMCABINETTABLE SET dlID = 8718968878589280256+dwID WHERE dlID=0

	IF EXISTS(SELECT * FROM TCASHBONUSITEMCHART WHERE wCashItemID = @wCashItemID)
	BEGIN
		IF(@wUseTime > 0)
			INSERT INTO TCASHITEMCABINETTABLE(
				dwUserID, wItemID, bLevel, bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime,bGradeEffect,
				bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6,
				wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
				dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, bGem, wMoggItemID)
			SELECT @dwRealTarget, CS.wItemID, CS.bLevel, CB.bBonusItemCount, CS.bGLevel, CS.dwDuraMax, CS.dwDuraCur, CS.bRefineCur, @dSetTime+wUseTime, bGradeEffect,
				bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6,
				wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
				dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, 0, 0
			FROM TCASHSHOPITEMCHART AS CS INNER JOIN TCASHBONUSITEMCHART AS CB ON CS.wID = CB.wBonusItem WHERE CB.wCashItemID = @wCashItemID
		ELSE
			INSERT INTO TCASHITEMCABINETTABLE(
				dwUserID, wItemID, bLevel, bCount, bGLevel, dwDuraMax, dwDuraCur, bRefineCur, dEndTime,bGradeEffect,
				bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6,
				wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
				dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, bGem, wMoggItemID)
			SELECT @dwRealTarget, CS.wItemID, CS.bLevel, CB.bBonusItemCount, CS.bGLevel, CS.dwDuraMax, CS.dwDuraCur, CS.bRefineCur, 0,bGradeEffect,
				bMagic1, bMagic2, bMagic3, bMagic4, bMagic5, bMagic6,
				wValue1, wValue2, wValue3, wValue4, wValue5, wValue6,
				dwTime1, dwTime2, dwTime3, dwTime4, dwTime5, dwTime6, 0, 0
			FROM TCASHSHOPITEMCHART AS CS INNER JOIN TCASHBONUSITEMCHART AS CB ON CS.wID = CB.wBonusItem WHERE CB.wCashItemID = @wCashItemID

		UPDATE TCASHITEMCABINETTABLE SET dlID = 8718968878589280256+dwID WHERE dlID=0
	END
END

RETURN @nRet
GO

CREATE OR ALTER PROCEDURE [dbo].[TCashCabinetBuy]
@dateTime SMALLDATETIME OUTPUT,
@dwCash INT OUTPUT,
@dwCashBonus INT OUTPUT,
@dwUserID INT,
@wCashItemID INT
AS
DECLARE @wUseTime INT
DECLARE @dwMoney INT
DECLARE @bSaleValue INT
DECLARE @dCurTime SMALLDATETIME
DECLARE @dCabinetUse SMALLDATETIME

SET @dateTime = NULL
SET @dwCash = 0
SET @dwCashBonus = 0

SELECT @wUseTime = wUseTime, @dwMoney = dwMoney, @bSaleValue = bSaleValue FROM TCASHSHOPITEMCHART WHERE wID = @wCashItemID
IF (@@ROWCOUNT = 0)
	RETURN 5 -- Not Found
SET @dwMoney = @dwMoney * (100 - @bSaleValue) / 100

BEGIN TRAN BUYCASHCABINET
EXEC TCashGet @dwUserID, @dwCash OUTPUT, @dwCashBonus OUTPUT
IF (ISNULL(@dwCash, 0) < @dwMoney)
BEGIN
	ROLLBACK TRAN BUYCASHCABINET
	SET @dwCash = ISNULL(@dwCash, 0)
	SET @dwCashBonus = ISNULL(@dwCashBonus, 0)
	RETURN 3 -- Need Cash
END
SET @dwCash = @dwCash - @dwMoney
UPDATE TCASHTESTTABLE SET dwCash = @dwCash WHERE dwUserID = @dwUserID

SET @dCurTime = GetDate()
SELECT @dCabinetUse = dCabinetUse FROM TUSERINFOTABLE WHERE dwUserID = @dwUserID
IF (@@ROWCOUNT = 0)
BEGIN
	SET @dateTime = DATEADD(DAY, @wUseTime, @dCurTime)
	INSERT INTO TUSERINFOTABLE (dwUserID, bCanCreateCharCount, bAgreement, dCabinetUse) VALUES(@dwUserID, 6, 1, @dateTime)
END
ELSE
BEGIN
	IF (@dCabinetUse IS NULL OR @dCabinetUse < @dCurTime)
		SET @dateTime = DATEADD(DAY, @wUseTime, @dCurTime)
	ELSE
		SET @dateTime = DATEADD(DAY, @wUseTime, @dCabinetUse)
	UPDATE TUSERINFOTABLE SET dCabinetUse = @dateTime WHERE dwUserID = @dwUserID
END
COMMIT TRAN BUYCASHCABINET
RETURN 0
GO

USE [TGame_gsp];
GO

CREATE OR ALTER PROCEDURE [dbo].[TCashCabinetBuy]
@dateTime SMALLDATETIME OUTPUT,
@dwCash INT OUTPUT,
@dwBonus INT OUTPUT,
@wCashItemID SMALLINT,
@dwCharID INT
AS
DECLARE @dwUserID INT
DECLARE @nRet INT
SET @dateTime = NULL
SET @dwCash = 0
SET @dwBonus = 0
SELECT @dwUserID = dwUserID FROM TCHARTABLE WHERE dwCharID = @dwCharID
IF(@@ROWCOUNT <> 1)
	RETURN 4
EXEC @nRet = TGLOBAL_GSP.DBO.TCashCabinetBuy @dateTime OUTPUT, @dwCash OUTPUT, @dwBonus OUTPUT, @dwUserID, @wCashItemID
RETURN @nRet
GO

CREATE OR ALTER PROCEDURE [dbo].[TGainCashBonus]
@dwCharID INT,
@bInven TINYINT,
@bItemID TINYINT,
@dwBonus INT
AS
DECLARE @nRet INT
DECLARE @dwUserID INT
SET @dwUserID = 0
SELECT @dwUserID = dwUserID FROM TCHARTABLE WHERE dwCharID = @dwCharID
IF(@@ROWCOUNT = 0 OR @dwUserID = 0)
	RETURN 1
DELETE TITEMTABLE WHERE dwOwnerID=@dwCharID AND bOwnerType=0 AND bStorageType=0 AND dwStorageID=@bInven AND bItemID = @bItemID
EXEC @nRet = TGLOBAL_GSP.DBO.TGainCashBonus @dwUserID, @dwBonus
RETURN ISNULL(@nRet, 0)
GO
