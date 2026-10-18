-- 020_save_tax_cap
--
-- TSaveTax adds a tax to an NPC's running total (dbo.TTAXTABLE.dwMoney, INT). In the baseline two totals are already within a few
-- thousand of the INT maximum (country 0 / NPC 20789 and country 1 / NPC 20841, the auctioneers), so the next addition overflowed:
-- "Arithmetic overflow error converting expression to data type int" — and since TAuctionReg, TAuctionBuyDirect and TAuctionEnd
-- all call it, every auction there failed. This keeps the proc's behaviour but adds in BIGINT and caps the total at the INT maximum.
--
-- Idempotent: CREATE OR ALTER.
USE [TGame_gsp];
GO

CREATE OR ALTER PROCEDURE [dbo].[TSaveTax]
    @bType      TINYINT,
    @bCountry   TINYINT,
    @wNpcID     SMALLINT,
    @dwAddMoney INT
AS
DECLARE @dlMoney BIGINT
DECLARE @dlAdd   BIGINT = ABS(CAST(@dwAddMoney AS BIGINT))
SELECT @dlMoney = dwMoney FROM TTAXTABLE WHERE bCountry = @bCountry AND wNpcID = @wNpcID
IF (@@ROWCOUNT = 0)
    INSERT INTO TTAXTABLE (bCountry, wNpcID, dwMoney) VALUES (@bCountry, @wNpcID, CASE WHEN @dlAdd > 2147483647 THEN 2147483647 ELSE @dlAdd END)
ELSE
BEGIN
    SET @dlMoney = @dlMoney + @dlAdd
    IF (@dlMoney < 0) SET @dlMoney = 0
    IF (@dlMoney > 2147483647) SET @dlMoney = 2147483647
    UPDATE TTAXTABLE SET dwMoney = CAST(@dlMoney AS INT) WHERE bCountry = @bCountry AND wNpcID = @wNpcID
END
GO
