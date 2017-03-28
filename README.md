# TopViewMovementComponent  

UnrealEngine4で、見下ろし視点(XY平面上)の移動を行うComponentの実装です。 
名前の通りPawn用です。Actorでは使えません。次のblog記事と連動したものになります。
[見下ろし2Dゲームの移動処理(TopViewMovement)を作る](http://katze.hatenablog.jp/entry/2017/03/28/205434)
  
# インストール  

1. 使用したいプロジェクトで、C++のプロジェクトを作り、Source/プロジェクト名 フォルダにコピーします。
1. C++をプロジェクトを再生成します
1. TopViewPawnMovementComponent.h の19行目 "MYPROJECT_API" の部分を、"プロジェクト名_API"に書き換える  
  例えば、ActionGameという名前でプロジェクトを作った場合は、"ACTIONGAME_API" にします
1. リビルドします  

# 使い方  
PawnにTopViewPawnMovementComponentを追加します。  
すべてBPから使用することができます。  

## 方向  
上方向を1として、時計回りに1ずつ増やした1～8の数値で扱います。  

## ナビゲーション移動  
AIMoveToなどナビゲーション移動ノードがそのまま使えます。  
ナビゲーションの中止もControllerのStopMovementを呼ぶことで中止します。  
  
## 毎Tick呼ぶ移動  
PawnのTickで毎回呼ぶことを前提にした移動関数です  

- MoveToDirection  
  - 指定した方向に移動します。FloatingPawnMovementComponentのAddInputVectorに相当します。  
- MoveToLocation  
  - 指定した位置への向きを自動で計算して移動します。ナビゲーション移動ではありません。単に最短距離で計算されます。  

## 自動移動
一度呼ぶと移動が終了するまで、移動し続けます。  

- MoveToDirectionDuration
  - 指定方向に指定時間(秒)だけ移動する
- MoveToLocationEase　
  - 指定位置に指定時間(秒)でEase移動する
- MoveToDirectionDistanceEase
  - 指定方向と距離を指定時間(秒)でEase移動する
- MoveToLocationAuto
  - 指定位置に向かって移動し続ける。ナビゲーション移動ではありません。
  
移動実行中は、MoveToDirection/MoveToLocationを呼び出すと移動が合成されてしまいますので注意してください。  
移動実行は、StopTopViewMovementを呼ぶことで中止することができます。  

# イベントディスパッチャー  
4つのイベントディスパッチャーを用意しています。

## OnStand  
移動が終わった時などに呼ばれます。

## OnMove  
移動中毎Tick呼ばれます。  

## OnNavDirection  
ナビゲーション移動中に向きが変更された時に呼ばれます。  
変更後の向きが引数に渡されます。  

## OnMoveFinished  
MoveToDirectionDuration/MoveToLocationEase/MoveToDirectionDistanceEase/MoveToLocationAuto で移動が目的地につくなどをして処理が終了した時に呼ばれます。  
正常終了した場合はAbort引数にfalseが渡されます。StopTopViewMovemntで途中終了した場合はtrueが渡されます。
