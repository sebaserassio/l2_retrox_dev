
USE [lin2world]
GO

ALTER  TABLE user_data add 
	liga_pvp_puntos_total int NOT NULL DEFAULT 0
go

ALTER  TABLE user_data add 
	liga_pvp_puntos_temporal int NOT NULL DEFAULT 0
go

ALTER  TABLE user_data add 
	liga_pvp_categoria int NOT NULL DEFAULT 0
go

ALTER TABLE dbo.user_data ADD
	liga_pvp_hardware_id nvarchar(50) NOT NULL DEFAULT N'00000000000000000000000000000000'
GO





USE [lin2world]
GO
/****** Object:  Table [dbo].[pvp_event_datos]    Script Date: 05/09/2023 02:47:13 a.m. ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

CREATE TABLE [dbo].[pvp_event_datos](
	[torneo_id] [int] NOT NULL,
	[char_id] [int] NOT NULL,
	[pos] [int] NOT NULL,
	[ronda] [int] NOT NULL,
	[descalificado] [int] NOT NULL
) ON [PRIMARY]

GO




USE [lin2world]
GO

/****** Object:  Table [dbo].[pvp_event]    Script Date: 05/09/2023 02:46:07 a.m. ******/
SET ANSI_NULLS ON
GO

SET QUOTED_IDENTIFIER ON
GO

SET ANSI_PADDING ON
GO

CREATE TABLE [dbo].[pvp_event](
	[torneo_id] [int] IDENTITY(1,1) NOT NULL,
	[fecha] [int] NOT NULL,
	[titulo] [varchar](200) NULL,
 CONSTRAINT [PK_pvp_event_datos] PRIMARY KEY CLUSTERED 
(
	[torneo_id] ASC
)WITH (PAD_INDEX = OFF, STATISTICS_NORECOMPUTE = OFF, IGNORE_DUP_KEY = OFF, ALLOW_ROW_LOCKS = ON, ALLOW_PAGE_LOCKS = ON) ON [PRIMARY]
) ON [PRIMARY]

GO

SET ANSI_PADDING OFF
GO








USE [lin2world]
GO
/****** Object:  StoredProcedure [dbo].[lin_PvPEventSetPlayerStatus]    Script Date: 05/09/2023 02:50:41 a.m. ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[lin_PvPEventSetPlayerStatus]
(
    @EventID INT,
    @charId INT,
    @pos INT,
    @ronda INT,
    @descalificado INT
)
AS
BEGIN
    SET NOCOUNT ON;

    -- Verificar si el evento existe en la tabla pvp_event
    IF NOT EXISTS (SELECT 1 FROM pvp_event WHERE torneo_id = @EventID)
    BEGIN
        -- Insertar un nuevo registro en la tabla pvp_event con los valores predeterminados

		-- Obtener la fecha y hora actual
		DECLARE @DateTime DATETIME = GETDATE();

		-- Convertir la fecha y hora a formato Unix (segundos desde el 1 de enero de 1970)
		DECLARE @UnixTime INT = DATEDIFF(SECOND, '19700101', @DateTime);

		-- Ahora puedes usar @UnixTime en tu inserción
		INSERT INTO pvp_event ([fecha], [titulo])
		VALUES (@UnixTime, '');


--		INSERT INTO pvp_event ([fecha], [titulo])
--		VALUES (CONVERT(INT, CONVERT(VARCHAR(8), GETDATE(), 112)), '');

        -- Obtener el nuevo ID del evento insertado
        SET @EventID = SCOPE_IDENTITY();
    END

    -- Verificar si el registro existe en la tabla pvp_event_datos
    IF NOT EXISTS (SELECT 1 FROM pvp_event_datos WHERE torneo_id = @EventID AND char_id = @charId)
    BEGIN
        -- Insertar un nuevo registro en la tabla pvp_event_datos
        INSERT INTO pvp_event_datos (torneo_id, char_id, pos, ronda, descalificado)
        VALUES (@EventID, @charId, @pos, @ronda, @descalificado);
    END
    ELSE
    BEGIN
        -- Actualizar los valores en la tabla pvp_event_datos
        UPDATE pvp_event_datos
        SET ronda = @ronda,
            descalificado = @descalificado
        WHERE torneo_id = @EventID AND char_id = @charId;
    END

    -- El resto de la lógica de tu procedimiento aquí
    -- ...

END

GO




USE [lin2world]
GO
/****** Object:  StoredProcedure [dbo].[lin_PvpEventLoadEvent]    Script Date: 05/09/2023 02:43:16 a.m. ******/
SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

CREATE PROCEDURE [dbo].[lin_PvpEventLoadEvent]
(
 @event_id INT
)
AS
SET NOCOUNT ON;
SELECT  ISNULL(MAX(torneo_id), 0) FROM pvp_event;



