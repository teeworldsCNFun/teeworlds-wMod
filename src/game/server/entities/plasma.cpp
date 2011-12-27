/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/event.h>
#include "plasma.h"
#include "turret.h"
#include "teleporter.h"
#include "explodewall.h"

CPlasma::CPlasma(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner)
    : CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
    m_Pos = Pos;
    m_Owner = Owner;
    m_Energy = StartEnergy;
    m_Dir = Direction;
    m_Bounces = 0;
    m_Vel = 1;
    GameWorld()->InsertEntity(this);
    Tick();
}


bool CPlasma::HitCharacter(vec2 From, vec2 To)
{
    vec2 At;
    CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
    CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);
    if(pHit)
    {
        m_Pos = At;
        if ( !GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) )
            m_Energy = -1;
        if ( !GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 )
            pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE, false);
        return true;
    }
    else
    {
        CTurret *pHitTurret = (CTurret *)GameServer()->m_World.IntersectEntity(m_Pos, To, 0.f, At, CGameWorld::ENTTYPE_TURRET);
        if (pHitTurret && pHitTurret->GetOwner() != m_Owner)
        {
            m_Pos = At;
            if ( !GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) )
                m_Energy = -1;
            if ( !GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 )
                pHitTurret->TakeDamage(GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE, false);
            return true;
        }
        else
        {
            CExplodeWall *pHitExplodeWall = 0;

            {
                CExplodeWall *p = (CExplodeWall *)GameWorld()->FindFirst(CGameWorld::ENTTYPE_EXPLODEWALL);
                for(; p; p = (CExplodeWall *)p->TypeNext())
                {
                    // Store the values for fast access and easy
                    // equations-to-code conversion
                    float x1 = From.x, x2 = To.x, x3 = p->m_From.x, x4 = p->m_Pos.x;
                    float y1 = From.y, y2 = To.y, y3 = p->m_From.y, y4 = p->m_Pos.y;

                    float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
                    // If d is zero, there is no intersection
                    if (d == 0)
                        continue;

                    // Get the x and y
                    float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
                    float x = ( pre * (x3 - x4) - (x1 - x2) * post ) / d;
                    float y = ( pre * (y3 - y4) - (y1 - y2) * post ) / d;

                    // Check if the x and y coordinates are within both lines
                    if ( x < min(x1, x2) || x > max(x1, x2) || x < min(x3, x4) || x > max(x3, x4) )
                        continue;
                    if ( y < min(y1, y2) || y > max(y1, y2) || y < min(y3, y4) || y > max(y3, y4) )
                        continue;

                    At.x = x;
                    At.y = y;
                    pHitExplodeWall = p;
                    break;
                }
           }

            if (pHitExplodeWall)
            {
                m_Pos = At;
                if ( !GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) )
                    m_Energy = -1;
                if ( !GameServer()->m_pEventsGame->IsActualEvent(WALLSHOT) || m_Bounces > 0 )
                    pHitExplodeWall->TakeDamage(GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE, false);
                return true;
            }
        }
    }

    return false;
}

void CPlasma::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CPlasma::Tick()
{
    if(m_Energy < 0 || GameLayerClipped(m_Pos))
    {
        GameServer()->m_World.DestroyEntity(this);
        return;
    }

    vec2 To = m_Pos + (m_Dir * m_Vel);
    m_Energy -= distance(m_Pos, To);
    if ( !GameServer()->m_pEventsGame->IsActualEvent(WEAPON_SLOW) )
        m_Vel += 0.5f;
    else
        m_Vel += 0.01f;

    vec2 At;
    CTeleporter *pTeleporter = (CTeleporter *)GameServer()->m_World.IntersectEntity(m_Pos, To, 12.0f, At, CGameWorld::ENTTYPE_TELEPORTER);
    if (pTeleporter && pTeleporter->GetNext())
    {
        if(!HitCharacter(m_Pos, To))
            m_Pos = pTeleporter->GetNext()->m_Pos + (m_Dir * 20.0f);
    }
    else if(!GameServer()->m_pEventsGame->IsActualEvent(BULLET_PIERCING) && !GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE) && GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
    {
        if(!HitCharacter(m_Pos, To))
        {
            // intersected
            m_Pos = To;

            vec2 TempPos = m_Pos;
            vec2 TempDir = m_Dir * 4.0f;

            GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
            m_Pos = TempPos;
            m_Dir = normalize(TempDir);

            m_Bounces++;
            m_Energy -= GameServer()->Tuning()->m_LaserBounceCost;

            if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
                m_Energy = -1;

            GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);
            GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_RIFLE, false, false);
        }
    }
    else
    {
        if(!HitCharacter(m_Pos, To))
        {
            if (GameServer()->m_pEventsGame->IsActualEvent(BULLET_GLUE))
                GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To);
            m_Pos = To;
        }
    }
}

void CPlasma::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient))
        return;

    CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
    if(!pObj)
        return;

    pObj->m_X = (int)m_Pos.x;
    pObj->m_Y = (int)m_Pos.y;
    pObj->m_FromX = (int)m_Pos.x;
    pObj->m_FromY = (int)m_Pos.y;
    pObj->m_StartTick = Server()->Tick();
}
