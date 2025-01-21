/*! \file IApplication.hpp
    \author Alan Ramirez
    \date 2024-09-21
    \brief Application meant to be run by Hush
*/

#include "IApplication.hpp"

void Hush::IApplication::Init()
{
    this->UserInit();
}

void Hush::IApplication::Update(float delta)
{
    this->UserUpdate(delta);

    m_scene->Update(delta);
}

void Hush::IApplication::FixedUpdate(float delta)
{
    this->UserFixedUpdate(delta);

    m_scene->FixedUpdate(delta);
}

void Hush::IApplication::OnPreRender()
{
    this->UserOnPreRender();

    m_scene->PreRender();
}

void Hush::IApplication::OnRender()
{
    this->UserOnRender();

    m_scene->Render();
}

void Hush::IApplication::OnPostRender()
{
    this->UserOnPostRender();

    m_scene->PostRender();
}
