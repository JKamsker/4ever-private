-- 021_char_table_pw_view
--
-- Three baseline procs read dbo.TCHARTABLE_PW, which the baseline database does not have: THeroSelect (naming a fort's or castle's
-- hero), TCharSetAsk and THalloweenEventPost. They failed with "Invalid object name 'TCHARTABLE_PW'", so no hero could ever be
-- named (HSR_INTERNAL). The procs only read dwCharID, szName and bCountry, which TCHARTABLE has; this adds TCHARTABLE_PW as a view
-- over it.
--
-- Idempotent: CREATE OR ALTER.
USE [TGame_gsp];
GO

CREATE OR ALTER VIEW [dbo].[TCHARTABLE_PW]
AS
SELECT * FROM [dbo].[TCHARTABLE];
GO
