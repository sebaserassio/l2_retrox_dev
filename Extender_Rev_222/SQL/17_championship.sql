USE [lin2world]
GO

-- Championship system
-- championship_team: id, name, char1_id, char2_id, char3_id, points, win_count, lose_count, draw_count, kill_count, death_count
-- championship_match: id, state, start_time, team1_id, team2_id, winner_id
-- state
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[championship_match]') AND type in (N'U'))
BEGIN
CREATE TABLE [dbo].[championship_match](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[state] [int] NOT NULL DEFAULT 0,	-- 0 - waiting, 1 - teleporting, 2 - preparing, 3 - fighting, 4 - done
	[start_time] [int] NOT NULL DEFAULT 0,
	[team1_id] [int] NOT NULL,
	[team2_id] [int] NOT NULL,
	[winner_id] [int] NOT NULL DEFAULT 0,
 CONSTRAINT [PK_championship_match] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
END
GO

IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[championship_team]') AND type in (N'U'))
BEGIN
CREATE TABLE [dbo].[championship_team](
	[id] [int] IDENTITY(1,1) NOT NULL,
	[name] VARCHAR(50),
	[char1_id] [int] NOT NULL,
	[char2_id] [int] NOT NULL,
	[char3_id] [int] NOT NULL,
	[points] [int] NOT NULL,
	[win_count] [int] NOT NULL,
	[lose_count] [int] NOT NULL,
	[draw_count] [int] NOT NULL,
	[kill_count] [int] NOT NULL,
	[death_count] [int] NOT NULL
 CONSTRAINT [PK_championship_team] PRIMARY KEY CLUSTERED 
(
	[id] ASC
)WITH (IGNORE_DUP_KEY = OFF) ON [PRIMARY]
) ON [PRIMARY]
END
GO


CREATE PROCEDURE lin_LoadChampionshipTeams
AS

SET NOCOUNT ON;
BEGIN
	SELECT [id], [name], [char1_id], [char2_id], [char3_id], [points], [win_count], [lose_count], [draw_count], [kill_count], [death_count] FROM [dbo].[championship_team]
END

go

CREATE PROCEDURE lin_SaveChampionshipTeam
(
	@id AS INT,
	@points AS INT,
	@win_count AS INT,
	@lose_count AS INT,
	@draw_count AS INT,
	@kill_count AS INT,
	@death_count AS INT
)
AS

SET NOCOUNT ON;
BEGIN
	UPDATE [dbo].[championship_team] SET [points] = @points, [win_count] = @win_count, [lose_count] = @lose_count, [draw_count] = @draw_count, [kill_count] = @kill_count, [death_count] = @death_count WHERE [id] = @id
END

go

CREATE PROCEDURE lin_LoadChampionshipMatches
AS
BEGIN
SET NOCOUNT ON;
	SELECT [id], [state], [start_time], [team1_id], [team2_id], [winner_id] FROM [dbo].[championship_match]
END

go

CREATE PROCEDURE lin_SaveChampionshipMatch
(
	@id AS INT,
	@state AS INT,
	@winner_id AS INT
)
AS

BEGIN
SET NOCOUNT ON;

UPDATE [dbo].[championship_match] SET [state] = @state, [winner_id] = @winner_id WHERE [id] = @id

END

go



CREATE PROCEDURE lin_RegisterChampionshipTeam
(
	@name AS VARCHAR(25),
	@char1_id AS INT,
	@char2_id AS INT,
	@char3_id AS INT
)
AS

SET NOCOUNT ON;
BEGIN
	INSERT INTO [championship_team] ([name], [char1_id], [char2_id], [char3_id]) VALUES (@name, @char1_id, @char2_id, @char3_id)
END

go