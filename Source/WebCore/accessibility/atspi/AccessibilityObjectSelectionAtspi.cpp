/*
 * Copyright (C) 2021 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "AccessibilityObjectAtspi.h"

#if ENABLE(ACCESSIBILITY) && USE(ATSPI)

#include "AccessibilityRootAtspi.h"

namespace WebCore {

GDBusInterfaceVTable AccessibilityObjectAtspi::s_selectionFunctions = {
    // method_call
    [](GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar* methodName, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer userData) {
        RELEASE_ASSERT(!isMainThread());
        auto atspiObject = Ref { *static_cast<AccessibilityObjectAtspi*>(userData) };
        atspiObject->updateBackingStore();

        if (!g_strcmp0(methodName, "GetSelectedChild")) {
            int index;
            g_variant_get(parameters, "(i)", &index);
            auto* child = index >= 0 ? atspiObject->selectedChild(index) : nullptr;
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(@(so))", child ? child->reference() : atspiObject->m_root.atspi().nullReference()));
        } else if (!g_strcmp0(methodName, "SelectChild")) {
            int index;
            g_variant_get(parameters, "(i)", &index);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", index >= 0 ? atspiObject->setChildSelected(index, true) : FALSE));
        } else if (!g_strcmp0(methodName, "DeselectSelectedChild")) {
            int index;
            g_variant_get(parameters, "(i)", &index);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", index >= 0 ? atspiObject->deselectSelectedChild(index) : FALSE));
        } else if (!g_strcmp0(methodName, "IsChildSelected")) {
            int index;
            g_variant_get(parameters, "(i)", &index);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", index >= 0 ? atspiObject->isChildSelected(index) : FALSE));
        } else if (!g_strcmp0(methodName, "SelectAll"))
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", atspiObject->selectAll()));
        else if (!g_strcmp0(methodName, "ClearSelection"))
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", atspiObject->clearSelection()));
        else if (!g_strcmp0(methodName, "DeselectChild")) {
            int index;
            g_variant_get(parameters, "(i)", &index);
            g_dbus_method_invocation_return_value(invocation, g_variant_new("(b)", index >= 0 ? atspiObject->setChildSelected(index, false) : FALSE));
        }
    },
    // get_property
    [](GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar* propertyName, GError** error, gpointer userData) -> GVariant* {
        RELEASE_ASSERT(!isMainThread());
        auto atspiObject = Ref { *static_cast<AccessibilityObjectAtspi*>(userData) };
        atspiObject->updateBackingStore();

        if (!g_strcmp0(propertyName, "NSelectedChildren"))
            return g_variant_new_int32(atspiObject->selectionCount());

        g_set_error(error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Unknown property '%s'", propertyName);
        return nullptr;
    },
    // set_property,
    nullptr,
    // padding
    nullptr
};

unsigned AccessibilityObjectAtspi::selectionCount() const
{
    return Accessibility::retrieveValueFromMainThread<unsigned>([this]() -> unsigned {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return 0;

        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        m_coreObject->selectedChildren(selectedItems);
        return selectedItems.size();
    });
}

AccessibilityObjectAtspi* AccessibilityObjectAtspi::selectedChild(unsigned index) const
{
    auto* wrapper = Accessibility::retrieveValueFromMainThread<AccessibilityObjectAtspi*>([this, index]() -> AccessibilityObjectAtspi* {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return nullptr;

        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        m_coreObject->selectedChildren(selectedItems);
        if (index >= selectedItems.size())
            return nullptr;

        return selectedItems[index]->wrapper();
    });

    return wrapper;
}

bool AccessibilityObjectAtspi::setChildSelected(unsigned index, bool selected) const
{
    return Accessibility::retrieveValueFromMainThread<bool>([this, index, selected]() -> bool {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return false;

        const auto& children = m_coreObject->children();
        if (index >= children.size())
            return false;

        if (!children[index]->canSetSelectedAttribute())
            return false;

        children[index]->setSelected(selected);
        return selected ? children[index]->isSelected() : !children[index]->isSelected();
    });
}

bool AccessibilityObjectAtspi::deselectSelectedChild(unsigned index) const
{
    return Accessibility::retrieveValueFromMainThread<bool>([this, index]() -> bool {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return false;

        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        m_coreObject->selectedChildren(selectedItems);
        if (index >= selectedItems.size())
            return false;

        if (!selectedItems[index]->canSetSelectedAttribute())
            return false;

        selectedItems[index]->setSelected(false);
        return !selectedItems[index]->isSelected();
    });
}

bool AccessibilityObjectAtspi::isChildSelected(unsigned index) const
{
    return Accessibility::retrieveValueFromMainThread<bool>([this, index]() -> bool {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return false;

        const auto& children = m_coreObject->children();
        if (index >= children.size())
            return false;

        return children[index]->isSelected();
    });
}

bool AccessibilityObjectAtspi::selectAll() const
{
    return Accessibility::retrieveValueFromMainThread<bool>([this]() -> bool {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return false;

        if (!m_coreObject->isMultiSelectable() || !m_coreObject->canSetSelectedChildren())
            return false;

        const auto& children = m_coreObject->children();
        unsigned selectableChildCount = 0;
        for (const auto& child : children) {
            if (child->canSetSelectedAttribute())
                selectableChildCount++;
        }

        if (!selectableChildCount)
            return false;

        m_coreObject->setSelectedChildren(children);
        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        m_coreObject->selectedChildren(selectedItems);
        return selectableChildCount == selectedItems.size();
    });
}

bool AccessibilityObjectAtspi::clearSelection() const
{
    return Accessibility::retrieveValueFromMainThread<bool>([this]() -> bool {
        if (m_coreObject)
            m_coreObject->updateBackingStore();

        if (!m_coreObject)
            return false;

        if (!m_coreObject->canSetSelectedChildren())
            return false;

        m_coreObject->setSelectedChildren({ });
        AccessibilityObject::AccessibilityChildrenVector selectedItems;
        m_coreObject->selectedChildren(selectedItems);
        return selectedItems.isEmpty();
    });
}

void AccessibilityObjectAtspi::selectionChanged()
{
    RELEASE_ASSERT(isMainThread());
    m_root.atspi().selectionChanged(*this);
}

} // namespace WebCore

#endif // ENABLE(ACCESSIBILITY) && USE(ATSPI)
