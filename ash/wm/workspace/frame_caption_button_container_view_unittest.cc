// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/frame_caption_button_container_view.h"

#include "ash/ash_switches.h"
#include "ash/test/ash_test_base.h"
#include "base/command_line.h"
#include "grit/ash_resources.h"
#include "ui/aura/root_window.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

namespace {

// Returns true if the images for |button|'s states match the passed in ids.
bool ImagesMatch(views::ImageButton* button,
                 int normal_image_id,
                 int hovered_image_id,
                 int pressed_image_id) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  gfx::ImageSkia* normal = rb.GetImageSkiaNamed(normal_image_id);
  gfx::ImageSkia* hovered = rb.GetImageSkiaNamed(hovered_image_id);
  gfx::ImageSkia* pressed = rb.GetImageSkiaNamed(pressed_image_id);
  using views::Button;
  return button->GetImage(Button::STATE_NORMAL).BackedBySameObjectAs(*normal) &&
      button->GetImage(Button::STATE_HOVERED).BackedBySameObjectAs(*hovered) &&
      button->GetImage(Button::STATE_PRESSED).BackedBySameObjectAs(*pressed);
}

// Returns true if |leftmost| and |rightmost| are flush with |container|'s
// edges.
bool CheckButtonsAtEdges(FrameCaptionButtonContainerView* container,
                         const views::ImageButton& leftmost,
                         const views::ImageButton& rightmost) {
  gfx::Rect container_size(container->GetPreferredSize());
  if (leftmost.y() == rightmost.y() &&
      leftmost.height() == rightmost.height() &&
      leftmost.x() == 0 &&
      leftmost.y() == 0 &&
      leftmost.height() == container_size.height() &&
      rightmost.bounds().right() == container_size.width()) {
    return true;
  }

  LOG(ERROR) << "Buttons " << leftmost.bounds().ToString() << " "
             << rightmost.bounds().ToString() << " not at edges of "
             << gfx::Rect(container_size).ToString();
  return false;
}

class TestWidgetDelegate : public views::WidgetDelegateView {
 public:
  TestWidgetDelegate(bool can_maximize) : can_maximize_(can_maximize) {
  }
  virtual ~TestWidgetDelegate() {
  }

  virtual bool CanMaximize() const OVERRIDE {
    return can_maximize_;
  }

 private:
  bool can_maximize_;

  DISALLOW_COPY_AND_ASSIGN(TestWidgetDelegate);
};

}  // namespace

class FrameCaptionButtonContainerViewTest : public ash::test::AshTestBase {
 public:
  enum MaximizeAllowed {
    MAXIMIZE_ALLOWED,
    MAXIMIZE_DISALLOWED
  };

  FrameCaptionButtonContainerViewTest() {
  }

  virtual ~FrameCaptionButtonContainerViewTest() {
  }

  // Creates a widget which allows maximizing based on |maximize_allowed|.
  // The caller takes ownership of the returned widget.
  views::Widget* CreateTestWidget(
      MaximizeAllowed maximize_allowed) WARN_UNUSED_RESULT {
    views::Widget* widget = new views::Widget;
    views::Widget::InitParams params;
    params.delegate = new TestWidgetDelegate(
        maximize_allowed == MAXIMIZE_ALLOWED);
    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    params.context = CurrentContext();
    widget->Init(params);
    return widget;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FrameCaptionButtonContainerViewTest);
};

// Test how the allowed actions affect which caption buttons are visible.
TEST_F(FrameCaptionButtonContainerViewTest, ButtonVisibility) {
  // The minimize button should be hidden when both minimizing and maximizing
  // are allowed because the size button can do both.
  scoped_ptr<views::Widget> widget_can_maximize(
      CreateTestWidget(MAXIMIZE_ALLOWED));
  FrameCaptionButtonContainerView container1(NULL, widget_can_maximize.get(),
      FrameCaptionButtonContainerView::MINIMIZE_ALLOWED);
  container1.Layout();
  FrameCaptionButtonContainerView::TestApi t1(&container1);
  EXPECT_FALSE(t1.minimize_button()->visible());
  EXPECT_TRUE(t1.size_button()->visible());
  EXPECT_TRUE(t1.close_button()->visible());
  EXPECT_TRUE(CheckButtonsAtEdges(
      &container1, *t1.size_button(), *t1.close_button()));

  // The minimize button should be visible when minimizing is allowed but
  // maximizing is disallowed.
  scoped_ptr<views::Widget> widget_cannot_maximize(
      CreateTestWidget(MAXIMIZE_DISALLOWED));
  FrameCaptionButtonContainerView container2(NULL, widget_cannot_maximize.get(),
      FrameCaptionButtonContainerView::MINIMIZE_ALLOWED);
  container2.Layout();
  FrameCaptionButtonContainerView::TestApi t2(&container2);
  EXPECT_TRUE(t2.minimize_button()->visible());
  EXPECT_FALSE(t2.size_button()->visible());
  EXPECT_TRUE(t2.close_button()->visible());
  EXPECT_TRUE(CheckButtonsAtEdges(
      &container2, *t2.minimize_button(), *t2.close_button()));

  // Neither the minimize button nor the size button should be visible when
  // neither minimizing nor maximizing are allowed.
  FrameCaptionButtonContainerView container3(NULL, widget_cannot_maximize.get(),
      FrameCaptionButtonContainerView::MINIMIZE_DISALLOWED);
  container3.Layout();
  FrameCaptionButtonContainerView::TestApi t3(&container3);
  EXPECT_FALSE(t3.minimize_button()->visible());
  EXPECT_FALSE(t3.size_button()->visible());
  EXPECT_TRUE(t3.close_button()->visible());
  EXPECT_TRUE(CheckButtonsAtEdges(
      &container3, *t3.close_button(), *t3.close_button()));

  // Neither the minimize button nor the size button should be visible when the
  // "force-maximize-mode" experiment is turned on.
  CommandLine::ForCurrentProcess()->AppendSwitch(switches::kForcedMaximizeMode);
  FrameCaptionButtonContainerView container4(NULL, widget_can_maximize.get(),
      FrameCaptionButtonContainerView::MINIMIZE_ALLOWED);
  container4.Layout();
  FrameCaptionButtonContainerView::TestApi t4(&container4);
  EXPECT_FALSE(t4.minimize_button()->visible());
  EXPECT_FALSE(t4.size_button()->visible());
  EXPECT_TRUE(t4.close_button()->visible());
  EXPECT_TRUE(CheckButtonsAtEdges(
      &container4, *t4.close_button(), *t4.close_button()));
}

// Test the layout when a border is set on the container.
TEST_F(FrameCaptionButtonContainerViewTest, LayoutBorder) {
  const int kTopInset = 1;
  const int kLeftInset = 2;
  const int kBottomInset = 3;
  const int kRightInset = 4;

  scoped_ptr<views::Widget> widget(CreateTestWidget(MAXIMIZE_ALLOWED));
  FrameCaptionButtonContainerView container(NULL, widget.get(),
      FrameCaptionButtonContainerView::MINIMIZE_ALLOWED);
  container.set_border(views::Border::CreateEmptyBorder(
      kTopInset, kLeftInset, kBottomInset, kRightInset));
  container.Layout();
  FrameCaptionButtonContainerView::TestApi t(&container);

  EXPECT_EQ(kLeftInset, t.size_button()->x());
  EXPECT_EQ(kTopInset, t.close_button()->y());
  EXPECT_EQ(container.GetPreferredSize().height(),
            t.close_button()->bounds().bottom() + kBottomInset);
  EXPECT_EQ(container.GetPreferredSize().width(),
            t.close_button()->bounds().right() + kRightInset);
}

// Test how the header style affects which images are used for the buttons.
TEST_F(FrameCaptionButtonContainerViewTest, HeaderStyle) {
  scoped_ptr<views::Widget> widget(CreateTestWidget(MAXIMIZE_ALLOWED));
  FrameCaptionButtonContainerView container(NULL, widget.get(),
      FrameCaptionButtonContainerView::MINIMIZE_ALLOWED);
  FrameCaptionButtonContainerView::TestApi t(&container);

  // Tall header style.
  container.set_header_style(
      FrameCaptionButtonContainerView::HEADER_STYLE_TALL);
  container.Layout();
  EXPECT_TRUE(ImagesMatch(t.size_button(),
                          IDR_AURA_WINDOW_MAXIMIZE,
                          IDR_AURA_WINDOW_MAXIMIZE_H,
                          IDR_AURA_WINDOW_MAXIMIZE_P));
  EXPECT_TRUE(ImagesMatch(t.close_button(),
                          IDR_AURA_WINDOW_CLOSE,
                          IDR_AURA_WINDOW_CLOSE_H,
                          IDR_AURA_WINDOW_CLOSE_P));

  // Short header style.
  container.set_header_style(
      FrameCaptionButtonContainerView::HEADER_STYLE_SHORT);
  container.Layout();
  EXPECT_TRUE(ImagesMatch(t.size_button(),
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE,
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE_H,
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE_P));
  EXPECT_TRUE(ImagesMatch(t.close_button(),
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE,
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE_H,
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE_P));

  // Maximized short header style.
  widget->Maximize();
  container.Layout();
  EXPECT_TRUE(ImagesMatch(t.size_button(),
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE2,
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE2_H,
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE2_P));
  EXPECT_TRUE(ImagesMatch(t.close_button(),
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE2,
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE2_H,
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE2_P));

  // The buttons are visible during a reveal of the top-of-window views in
  // immersive fullscreen. They should use the same images as maximized.
  widget->SetFullscreen(true);
  container.Layout();
  EXPECT_TRUE(ImagesMatch(t.size_button(),
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE2,
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE2_H,
                          IDR_AURA_WINDOW_MAXIMIZED_RESTORE2_P));
  EXPECT_TRUE(ImagesMatch(t.close_button(),
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE2,
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE2_H,
                          IDR_AURA_WINDOW_MAXIMIZED_CLOSE2_P));

  // AppNonClientFrameViewAsh has a dedicated set of images.
  container.set_header_style(
      FrameCaptionButtonContainerView::HEADER_STYLE_MAXIMIZED_HOSTED_APP);
  container.Layout();
  EXPECT_TRUE(ImagesMatch(t.size_button(),
                          IDR_AURA_WINDOW_FULLSCREEN_RESTORE,
                          IDR_AURA_WINDOW_FULLSCREEN_RESTORE_H,
                          IDR_AURA_WINDOW_FULLSCREEN_RESTORE_P));
  EXPECT_TRUE(ImagesMatch(t.close_button(),
                          IDR_AURA_WINDOW_FULLSCREEN_CLOSE,
                          IDR_AURA_WINDOW_FULLSCREEN_CLOSE_H,
                          IDR_AURA_WINDOW_FULLSCREEN_CLOSE_P));
}

}  // namespace ash